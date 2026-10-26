#include "backend.h"
#include "CDosStatusTask.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>

// Only the class name is adapted.  The method body comes verbatim from product source.
using CDebugInterface = CDebugInterfaceVice;
#include "enqueue_method.inc"
using namespace DosStatus;
extern "C" {
drive_context_t *drive_context[4]{};
}
static std::atomic<unsigned int> backendReads{0}, virtualReads{0};
extern "C" int resources_get_int(const char *, int *value)
{
	++backendReads;
	*value = 1;
	return 0;
}
extern "C" vdrive_t *file_system_get_vdrive(unsigned int)
{
	++virtualReads;
	return nullptr;
}

int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string mode = argv[2];
	std::ifstream input(argv[1], std::ios::binary);
	std::vector<uint8_t> rom((std::istreambuf_iterator<char>(input)), {});
	assert(rom.size() == 0x4000);
	CDebugInterfaceVice vice;
	drive_context_t drive;
	drive_t unit;
	drive.drive = &unit;
	drive_context[0] = &drive;
	// iecrom_setup_image duplicates the supported 16 KiB image at both halves.
	std::copy(rom.begin(), rom.end(), drive.rom);
	std::copy(rom.begin(), rom.end(), drive.rom + 0x4000);
	std::copy(std::begin(drive.rom), std::end(drive.rom), drive.trap_rom);
	const char text[] = "00,OK,00,00";
	std::memcpy(drive.drive_ram + 0x2D5, text, sizeof(text) - 1);
	drive.drive_ram[0xA5] = 0xD6; drive.drive_ram[0xA6] = 2;
	drive.drive_ram[0x249] = 0xD5 + sizeof(text) - 2;
	drive.drive_ram[0xF7] = 0x88;
	assert(Capture(&vice, 8).outcome == "complete");

	if (mode == "enqueue")
	{
		const auto destroyed = CDebugInterfaceTask::destroyed.load();
		const auto reads = backendReads.load();
		unsigned int injectedDepth = 0, allocations = 0;
		beforeTaskAllocation = [&] {
			++allocations;
			injectedDepth = vice.tasksMutex->Depth();
			assert(injectedDepth == 2); // Observe's outer TryLock, then Add's inner Lock.
			throw std::bad_alloc();
		};
		auto failed = Observe(&vice, 8);
		beforeTaskAllocation = {};
		bool destroyedOnce = CDebugInterfaceTask::destroyed == destroyed + 1;
		bool empty = vice.tasks.empty() && !vice.hasPendingCpuDebugInterruptTasks.load();
		bool noRead = backendReads == reads;
		auto foreign = std::async(std::launch::async, [&] {
			if (!vice.tasksMutex->TryLock()) return false;
			vice.tasksMutex->Unlock();
			return true;
		});
		assert(foreign.wait_for(std::chrono::seconds(1)) == std::future_status::ready);
		bool reacquired = foreign.get();
		// Same-thread admission also distinguishes released slot from a stuck busy slot.
		auto reused = Observe(&vice, 8);
		bool slotReused = reused.outcome == "timeout" && vice.tasks.size() == 1;
		vice.Drain(); // Expired work must retire without touching the backend.
		std::cout << "enqueue: depth=" << injectedDepth << " allocations=" << allocations
			<< " outcome=" << failed.outcome << " destroyed=" << destroyedOnce
			<< " empty=" << empty << " noRead=" << noRead << " slotReuse=" << slotReused
			<< " foreignReacquired=" << reacquired << " finalDepth=" << vice.tasksMutex->Depth()
			<< std::endl;
		assert(allocations == 1 && injectedDepth == 2);
		assert(failed.outcome == "unavailable");
		assert(destroyedOnce && empty && noRead && slotReused);
		assert(vice.tasks.empty() && CDebugInterfaceTask::destroyed == destroyed + 2);
		assert(backendReads == reads && vice.paused);
		assert(reacquired && vice.tasksMutex->Depth() == 0);
	}
	else
	{
		if (mode == "lower-current") drive.rom[0x0123] ^= 1;
		else if (mode == "lower-execution") drive.trap_rom[0x0123] ^= 1;
		else if (mode == "lower-both") { drive.rom[0x0123] ^= 1; drive.trap_rom[0x0123] ^= 1; }
		else if (mode == "lower-idle-trap")
		{
			drive.idling_method = DRIVE_IDLE_TRAP_IDLE;
			drive.trap_rom[0x2C9B] = 2; // AC9B is NOT the permitted EC9B patch.
		}
		else if (mode == "unknown-mirrored-rom")
		{
			for (auto i : {0x0123, 0x4123}) { drive.rom[i] ^= 1; drive.trap_rom[i] ^= 1; }
		}
		else if (mode == "upper-idle-disabled") drive.trap_rom[0x6C9B] = 2;
		else { assert(mode == "upper-idle-enabled"); drive.idling_method = DRIVE_IDLE_TRAP_IDLE; drive.trap_rom[0x6C9B] = 2; }
		auto result = Capture(&vice, 8);
		std::cout << mode << ": outcome=" << result.outcome << " reason=" << result.reason << std::endl;
		if (mode == "upper-idle-enabled") assert(result.outcome == "complete");
		else assert(result.outcome == "unsupported");
	}
	assert(virtualReads == 0);
}
