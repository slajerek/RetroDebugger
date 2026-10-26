#pragma once

#include "CDosStatus.h"
#include "CDebugInterfaceTask.h"
#include "SYS_Threading.h"
#include "sgSHA256/sgSHA256.h"

extern "C" {
#include "drive.h"
#include "drivetypes.h"
#include "resources.h"
// Windows RPC headers define small, but VICE uses it as a member name.
#ifdef _WIN32
#pragma push_macro("small")
#undef small
#endif
#include "vdrive.h"
#ifdef _WIN32
#pragma pop_macro("small")
#endif
#include "attach.h"
#include "cbmdos.h"
}

namespace DosStatus
{
// True means acquired in the actual SDL3/fallback engine implementations.
class TryLock
{
	CSlrMutex *mutex;
	bool acquired;
public:
	explicit TryLock(CSlrMutex *mutex) : mutex(mutex), acquired(mutex && mutex->TryLock()) {}
	~TryLock() { if (acquired) mutex->Unlock(); }
	explicit operator bool() const { return acquired; }
	TryLock(const TryLock &) = delete;
	TryLock &operator=(const TryLock &) = delete;
};

inline std::string HashRom(const uint8_t *rom)
{
	sgSHA256 sha;
	sha.update(rom, 0x4000);
	auto digest = sha.digest();
	const char hex[] = "0123456789abcdef";
	std::string result;
	for (auto b : digest)
	{
		result += hex[b >> 4];
		result += hex[b & 15];
	}
	return result;
}

inline Snapshot Capture(CDebugInterfaceVice *vice, unsigned int device)
{
	Snapshot s;
	TryLock interfaceLock(vice->breakpointsMutex);
	if (!interfaceLock)
		return Refuse(s, "unavailable", "interface lock contended");
	TryLock ioLock(vice->ioMutex);
	if (!ioLock)
		return Refuse(s, "unavailable", "IO lock contended");
	int trueDrive = 0;
	if (resources_get_int("DriveTrueEmulation", &trueDrive) < 0)
		return Refuse(s, "unavailable", "drive mode unavailable");
	if (trueDrive)
	{
		s.backend = "true-drive-rom-workspace";
		auto *drv = drive_context[device - 8];
		if (!drv || !drv->drive || !drv->enable)
			return Refuse(s, "unavailable", "true-drive device is disabled or absent; no virtual fallback");
		s.driveType = drv->type;
		s.mediaPresent = drv->drive->image != nullptr;
		s.mediaKnown = true;
		if ((drv->type != DRIVE_TYPE_1541 && drv->type != DRIVE_TYPE_1541II)
			|| drv->drive->type != drv->type)
			return Refuse(s, "unsupported", "only exact stock 1541/1541-II firmware layouts are supported");
		s.raw.assign(drv->drive_ram + 0x2D5, drv->drive_ram + 0x2F9);
		s.pointer = drv->drive_ram[0xA5] | (drv->drive_ram[0xA6] << 8);
		s.end = drv->drive_ram[0x249];
		s.channelState = drv->drive_ram[0xF7];
		s.firmwareSha256 = HashRom(drv->rom + 0x4000);
		const char *expectedFirmware = drv->type == DRIVE_TYPE_1541
			? "d1d45afb46fd4e2b48d93ca367b889d75654a3b7acf73044e51f6d880c09369e"
			: "326c289c38753323d7e8167897447cf61ef35189d82eb8d75210ece949adda7c";
		if (s.firmwareSha256 != expectedFirmware
			|| drv->profdos || drv->supercard || drv->stardos || drv->ds1216
			|| drv->drive_ram2_enabled || drv->drive_ram4_enabled || drv->drive_ram6_enabled
			|| drv->drive_ram8_enabled || drv->drive_rama_enabled)
			return Refuse(s, "unsupported", "unrecognized firmware or expanded memory mapping");
		// Stock 16 KiB 1541/1541-II ROM is mirrored at $8000-$BFFF (iecrom.c/memiec.c).
		// Qualify both data reads and fast fetches, with no lower-mirror trap exception.
		for (unsigned int i = 0; i < 0x4000; i++)
		{
			if (drv->rom[i] != drv->rom[i + 0x4000]
				|| drv->trap_rom[i] != drv->rom[i + 0x4000])
				return Refuse(s, "unsupported", "lower ROM mirror differs from supported stock mapping");
		}
		// CPU fast fetches use trap_rom.  Allow only VICE's known idle JMP patch
		// (driverom.c), not arbitrary differences hidden by an unchanged ROM.
		for (unsigned int i = 0x4000; i < 0x8000; i++)
		{
			if (drv->trap_rom[i] == drv->rom[i])
				continue;
			if (i == 0x6C9B && drv->trap_rom[i] == 0x02
				&& drv->idling_method == DRIVE_IDLE_TRAP_IDLE)
				continue;
			return Refuse(s, "unsupported", "execution ROM differs from supported stock/trap mapping");
		}
		return ParseStock1541(std::move(s));
	}
	s.backend = "virtual-command-channel";
	auto *vdrive = file_system_get_vdrive(device);
	if (!vdrive || !vdrive->buffers[15].buffer)
		return Refuse(s, "unavailable", "virtual command channel unavailable");
	const auto &reply = vdrive->buffers[15];
	s.mediaPresent = vdrive->image != nullptr;
	s.mediaKnown = true;
	s.mode = reply.mode;
	s.readMode = reply.readmode;
	s.pointer = reply.bufptr;
	s.end = reply.length;
	s.lastCode = vdrive->last_code;
	// Copy the bounded initialized reply range only, not uninitialized tail data.
	if (reply.length > 255)
		return Refuse(s, "unsupported", "virtual reply exceeds bounded status representation");
	s.raw.assign(reply.buffer, reply.buffer + reply.length + 1);
	return ParseVirtual(std::move(s), BUFFER_COMMAND_CHANNEL, CBMDOS_FAM_READ);
}

class Task : public CDebugInterfaceTask
{
	CDebugInterfaceVice *vice;
	unsigned int device;
	std::shared_ptr<Slot> slot;
	std::shared_ptr<Request> request;
public:
	Task(CDebugInterfaceVice *vice, unsigned int device, std::shared_ptr<Slot> slot,
		std::shared_ptr<Request> request) : vice(vice), device(device), slot(slot), request(request) {}
	~Task() override { slot->Retire(request); }
	void ExecuteTask() override
	{
		if (!request->Start())
			return;
		try
		{
			// Never hold the request mutex while acquiring locks or copying VICE.
			request->Finish(Capture(vice, device));
		}
		catch (...)
		{
			request->Finish(Refuse(Snapshot(), "unavailable", "status capture failed"));
		}
	}
};

inline Snapshot Observe(CDebugInterfaceVice *vice, unsigned int device)
{
	static auto slot = std::make_shared<Slot>();
	auto request = slot->Reserve(Clock::now() + std::chrono::milliseconds(250));
	if (!request)
		return Refuse(Snapshot(), "busy", "one status task remains outstanding");
	try
	{
		std::unique_ptr<Task> task(new Task(vice, device, slot, request));
		{
			TryLock queueLock(vice->tasksMutex);
			if (!queueLock)
				return Refuse(Snapshot(), "busy", "CPU task queue lock contended");
			// tasksMutex is recursive.  Queue owns task only after successful add.
			vice->AddCpuDebugInterruptTask(task.get());
			task.release();
		}
	}
	catch (...)
	{
		slot->Retire(request);
		return Refuse(Snapshot(), "unavailable", "status admission failed");
	}
	return request->Wait();
}
}
