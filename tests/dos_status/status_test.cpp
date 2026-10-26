#include "backend.h"
#include "CDosStatusTask.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <thread>

using namespace DosStatus;
extern "C" {
drive_context_t *drive_context[4]{};
}
static vdrive_t virtualDrive;
static int trueDrive = 0, resourceError = 0;
static std::atomic<int> reads{0}, virtualReads{0};
static std::function<void()> readHook;
extern "C" int resources_get_int(const char *, int *value)
{
	++reads;
	if (readHook) readHook();
	*value = trueDrive;
	return resourceError;
}
extern "C" vdrive_t *file_system_get_vdrive(unsigned int device)
{
	++virtualReads;
	return device >= 8 && device <= 11 ? &virtualDrive : nullptr;
}

static Snapshot Fixture(const std::string &text)
{
	Snapshot s;
	s.raw.assign(36, 0xCC);
	std::copy(text.begin(), text.end(), s.raw.begin());
	s.pointer = 0x2D6;
	s.end = 0xD5 + text.size() - 1;
	s.channelState = 0x88;
	return s;
}

int main(int argc, char **argv)
{
	assert(argc == 3);
	const unsigned int driveType = std::stoul(argv[2]);
	assert(driveType == DRIVE_TYPE_1541 || driveType == DRIVE_TYPE_1541II);
	for (const auto &text : {"00,OK,00,00", "74,DRIVE NOT READY,00,00", "73,CBM DOS V2.6 1541,00,00", "23,READ ERROR,18,01"})
	{
		auto s = Fixture(text), before = s;
		auto result = ParseStock1541(s);
		assert(result.outcome == "complete" && result.text == text);
		assert(s.raw == before.raw && s.pointer == before.pointer);
		s.raw[0] = 0;
		assert(result.raw[0] != 0);
	}
	for (auto pointer : {0u, 0x2D5u, 0x2F9u, 0x3D6u})
	{
		auto s = Fixture("00,OK,00,00"); s.pointer = pointer;
		assert(ParseStock1541(s).outcome == "transient");
	}
	for (auto end : {0u, 0xD4u, 0xF9u, 255u})
	{
		auto s = Fixture("00,OK,00,00"); s.end = end;
		assert(ParseStock1541(s).outcome == "transient");
	}
	auto s = Fixture("00,OK,00,00"); s.channelState = 0x80;
	assert(ParseStock1541(s).outcome == "transient");
	s = Fixture("00,OK,00,00"); s.raw[3] = 0xFF;
	assert(ParseStock1541(s).outcome == "unsupported");
	s = Fixture("00,OK,0X,00"); assert(ParseStock1541(s).outcome == "transient");
	s = Fixture("00,OK,00,00"); s.raw.resize(10);
	assert(ParseStock1541(s).outcome == "transient");

	CDebugInterfaceVice vice;
	uint8_t reply[256]{};
	const char text[] = "00,OK,00,00\r";
	std::memcpy(reply, text, sizeof(text));
	auto &b = virtualDrive.buffers[15]; b.buffer = reply; b.length = sizeof(text) - 2;
	auto original = std::vector<uint8_t>(reply, reply + 256);
	for (unsigned int device : {8, 9, 10, 11})
	{
		auto r = Capture(&vice, device);
		assert(r.outcome == "complete" && r.backend == "virtual-command-channel" && !r.mediaPresent);
		assert(b.bufptr == 0 && b.length == sizeof(text) - 2 && virtualDrive.last_code == 0);
		assert(std::equal(original.begin(), original.end(), reply));
		reply[0] = 1; assert(r.raw[0] == '0'); reply[0] = '0';
	}
	b.bufptr = 2; assert(Capture(&vice, 8).outcome == "complete" && b.bufptr == 2);
	b.bufptr = 250; assert(Capture(&vice, 8).outcome == "transient"); b.bufptr = 0;
	b.readmode = 1; assert(Capture(&vice, 8).outcome == "unsupported"); b.readmode = 0;
	b.mode = 0; assert(Capture(&vice, 8).outcome == "unsupported"); b.mode = 5;
	virtualDrive.last_code = 4; assert(Capture(&vice, 8).outcome == "unsupported");
	virtualDrive.last_code = 23; assert(Capture(&vice, 8).outcome == "unsupported"); virtualDrive.last_code = 0;
	reply[3] = 1; assert(Capture(&vice, 8).outcome == "unsupported"); reply[3] = 'O';
	b.length = 256; assert(Capture(&vice, 8).outcome == "unsupported"); b.length = sizeof(text) - 2;

	std::ifstream input(argv[1], std::ios::binary);
	std::vector<uint8_t> rom((std::istreambuf_iterator<char>(input)), {});
	assert(rom.size() == 16384);
	drive_context_t drive;
	drive_t unit;
	drive.drive = &unit;
	drive.type = unit.type = driveType;
	// VICE iecrom_setup_image mirrors the supported 16 KiB stock image.
	std::copy(rom.begin(), rom.end(), drive.rom);
	std::copy(rom.begin(), rom.end(), drive.rom + 0x4000);
	std::copy(std::begin(drive.rom), std::end(drive.rom), drive.trap_rom);
	s = Fixture("74,DRIVE NOT READY,00,00");
	std::copy(s.raw.begin(), s.raw.end(), drive.drive_ram + 0x2D5);
	drive.drive_ram[0xA5] = 0xD6; drive.drive_ram[0xA6] = 2;
	drive.drive_ram[0x249] = s.end; drive.drive_ram[0xF7] = 0x88;
	trueDrive = 1;
	int oldVirtualReads = virtualReads;
	assert(Capture(&vice, 8).outcome == "unavailable");
	drive_context[0] = &drive;
	auto ramBefore = std::vector<uint8_t>(std::begin(drive.drive_ram), std::end(drive.drive_ram));
	for (int i = 0; i < 2; ++i)
	{
		auto r = Capture(&vice, 8);
		assert(r.outcome == "complete" && r.code == 74 && !r.mediaPresent);
		assert(r.backend == "true-drive-rom-workspace" && r.raw == s.raw);
		assert(r.driveType == driveType && r.firmwareSha256 == HashRom(rom.data()));
	}
	assert(std::equal(ramBefore.begin(), ramBefore.end(), drive.drive_ram));
	drive.enable = 0; assert(Capture(&vice, 8).outcome == "unavailable"); drive.enable = 1;
	drive.type = 1571; assert(Capture(&vice, 8).outcome == "unsupported"); drive.type = driveType;
	unit.type = driveType == DRIVE_TYPE_1541 ? DRIVE_TYPE_1541II : DRIVE_TYPE_1541;
	assert(Capture(&vice, 8).outcome == "unsupported"); unit.type = driveType;
	// An admitted type pair must still match its own known stock ROM.
	drive.type = unit.type = driveType == DRIVE_TYPE_1541 ? DRIVE_TYPE_1541II : DRIVE_TYPE_1541;
	assert(Capture(&vice, 8).outcome == "unsupported"); drive.type = unit.type = driveType;
	drive.rom[0x4000] ^= 1; drive.trap_rom[0x4000] ^= 1;
	assert(Capture(&vice, 8).outcome == "unsupported"); // Same unknown ROM in both mappings.
	drive.rom[0x4000] ^= 1; drive.trap_rom[0x4000] ^= 1;
	// Keep the hash gate independently tested even when every mirror agrees.
	for (auto offset : {0, 0x4000}) { drive.rom[offset] ^= 1; drive.trap_rom[offset] ^= 1; }
	assert(Capture(&vice, 8).outcome == "unsupported");
	for (auto offset : {0, 0x4000}) { drive.rom[offset] ^= 1; drive.trap_rom[offset] ^= 1; }
	drive.trap_rom[0x4000] ^= 1; assert(Capture(&vice, 8).outcome == "unsupported"); drive.trap_rom[0x4000] ^= 1;
	drive.idling_method = DRIVE_IDLE_TRAP_IDLE; drive.trap_rom[0x6C9B] = 2;
	assert(Capture(&vice, 8).outcome == "complete");
	drive.drive_ram8_enabled = 1; assert(Capture(&vice, 8).outcome == "unsupported"); drive.drive_ram8_enabled = 0;
	assert(virtualReads == oldVirtualReads); // Active true drive NEVER consults vdrive.
	trueDrive = 0;
	resourceError = -1; assert(Capture(&vice, 8).outcome == "unavailable"); resourceError = 0;

	// Admission and callback contention.  Locks are held by another thread.
	for (auto *lock : {vice.tasksMutex, vice.breakpointsMutex, vice.ioMutex})
	{
		lock->Lock();
		auto result = std::async(std::launch::async, [&] {
			return lock == vice.tasksMutex ? Observe(&vice, 8) : Capture(&vice, 8);
		});
		assert(result.wait_for(std::chrono::seconds(1)) == std::future_status::ready);
		assert(result.get().outcome == (lock == vice.tasksMutex ? "busy" : "unavailable"));
		lock->Unlock();
	}
	assert(vice.tasks.empty());

	// A timed-out queued request retains the slot and performs no late read.
	int beforeReads = reads;
	assert(Observe(&vice, 8).outcome == "timeout");
	for (int i = 0; i < 20; ++i) assert(Observe(&vice, 8).outcome == "busy");
	assert(vice.tasks.size() == 1);
	vice.Drain(); assert(reads == beforeReads && vice.tasks.empty() && vice.paused);

	// Normal completion on a simulated CPU drain, without control-state changes.
	for (bool paused : {true, false})
	{
		vice.paused = paused;
		auto next = vice.adds + 1;
		auto result = std::async(std::launch::async, [&] { return Observe(&vice, 8); });
		vice.AwaitAdd(next); vice.Drain();
		assert(result.get().outcome == "complete" && vice.paused == paused);
	}

	// Backend already executing: requester expires WITHOUT backend/IO locks.
	std::promise<void> entered, release;
	auto released = release.get_future();
	readHook = [&] { entered.set_value(); released.wait(); };
	auto next = vice.adds + 1;
	auto result = std::async(std::launch::async, [&] { return Observe(&vice, 8); });
	vice.AwaitAdd(next);
	auto drain = std::async(std::launch::async, [&] { vice.Drain(); });
	entered.get_future().wait();
	assert(result.wait_for(std::chrono::seconds(1)) == std::future_status::ready);
	assert(result.get().outcome == "timeout" && Observe(&vice, 8).outcome == "busy");
	release.set_value(); drain.get(); readHook = {};

	// Identity-safe retirement and deterministic expiry/completion orderings.
	Slot slot;
	auto old = slot.Reserve(Clock::now() - std::chrono::milliseconds(1));
	assert(!old->Start() && old->Wait().outcome == "timeout");
	assert(!slot.Reserve(Clock::now())); slot.Retire(old);
	auto current = slot.Reserve(Clock::now() + std::chrono::seconds(1));
	slot.Retire(old); assert(!slot.Reserve(Clock::now()));
	assert(current->Start());
	s.outcome = "complete"; current->Finish(s);
	assert(current->Wait().outcome == "complete"); slot.Retire(current);
	// A preempted test thread may reach Start after the deadline; that is a valid expiry.
	for (int i = 0; i < 100; ++i)
	{
		auto r = std::make_shared<Request>(Clock::now() + std::chrono::milliseconds(1));
		bool started = r->Start();
		auto finish = std::async(std::launch::async, [r, s, started] { if (started) r->Finish(s); });
		auto observed = r->Wait(); finish.get();
		assert(observed.outcome == "complete" || observed.outcome == "timeout");
		assert(r->phase == Phase::completed || r->phase == Phase::expired);
		if (observed.outcome == "timeout") assert(r->result.raw.empty());
	}
	std::cout << "DOS status portable parser, qualification, copy, contention, and retirement checks passed\n";
}
