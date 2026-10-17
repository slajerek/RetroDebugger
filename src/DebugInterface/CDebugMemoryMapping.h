#ifndef _CDebugMemoryMapping_h_
#define _CDebugMemoryMapping_h_

// Logical vs physical code identity (RD-024, spec #3A.2 and #3B.1 of
// docs/specs/source-level-debugging).
//
// A CPU address is NOT an identity: on the C64 $A000 is BASIC ROM or RAM
// depending on the 6510 port, on the NES $8000 is whichever PRG bank the
// mapper selected, on the Atari XL/XE $C000 is OS ROM or RAM depending on
// PORTB. Everything that must survive a bank switch -- source mappings,
// labels, physical breakpoints -- binds to a CDebugPhysicalAddress; what the
// CPU sees at an instant is a CDebugMappedAddress produced by a machine's
// CDebugMemoryMappingResolver.
//
// Rules every resolver follows:
//  - Fetch, read and write are resolved separately: a C64 write under KERNAL
//    lands in RAM while fetch/read see ROM; an Atari XL/XE ROM blocks writes.
//  - The emulator is the authority. A resolver reads the core's real banking
//    state (PLA config, mapper bank pointers, PORTB/cartridge state); it never
//    re-simulates bank registers from the last byte written.
//  - Unknown is a first-class answer. When the resolver cannot tell, it says
//    DebugMemoryKind::Unknown; it never falls back to an arbitrary bank.

#include "SYS_Defs.h"
#include <string>
#include <vector>

enum class DebugMemoryAccess : u8
{
	Fetch,	// instruction fetch (opcode and operand bytes)
	Read,	// data read
	Write	// data write: the DESTINATION, which may differ from what a read sees
};

enum class DebugMemoryBusMaster : u8
{
	Cpu,
	Vic,	// C64 VIC-II
	Antic,	// Atari ANTIC
	Ppu	// NES PPU
};

enum class DebugMemoryKind : u8
{
	Unknown,		// the resolver cannot tell (not supported, or state unavailable)
	Unmapped,		// nothing answers this access (Ultimax holes); reads see open bus
	Ram,			// main RAM, including RAM under ROM
	Rom,			// machine ROM (BASIC, KERNAL, character ROM, OS)
	Io,				// I/O registers (VIC, SID, CIA, GTIA, PPU ports, ...)
	CpuPort,		// a register inside the CPU (6510 $0000/$0001)
	CartridgeRom,	// cartridge or PRG ROM
	CartridgeRam,	// cartridge or PRG RAM
	ExtendedRam		// banked expansion RAM (Atari 130XE and compatibles)
};

const char *DebugMemoryKindName(DebugMemoryKind kind);
const char *DebugMemoryAccessName(DebugMemoryAccess access);

// Where a byte physically lives: a backing object, a region in it, an offset.
// Stable across bank switches; this is what source maps and physical
// breakpoints bind to. Field meanings per machine are documented by each
// resolver; (device, kind, region, offset) together are the identity.
struct CDebugPhysicalAddress
{
	DebugMemoryKind kind = DebugMemoryKind::Unknown;

	// The backing object: "c64", "c64-cart", "nes-prg", "atari", "atari-cart"...
	// One name per object the resolver knows; empty only for Unknown/Unmapped.
	std::string device;

	// Chip, bank or region inside the device (cartridge bank, ROM chip,
	// 130XE bank). Its numbering belongs to the emulator, not to a compiler.
	u32 region = 0;

	// Byte offset inside that region.
	u32 offset = 0;

	bool IsKnown() const { return kind != DebugMemoryKind::Unknown && kind != DebugMemoryKind::Unmapped; }

	// RAM can be rewritten at runtime, so code there may diverge from the
	// debug artifact that described it; ROM cannot.
	bool IsMutable() const;

	bool operator==(const CDebugPhysicalAddress &other) const;
	bool operator!=(const CDebugPhysicalAddress &other) const { return !(*this == other); }
};

// What one access at one CPU address resolves to right now.
struct CDebugMappedAddress
{
	u16 cpuAddress = 0;
	DebugMemoryAccess access = DebugMemoryAccess::Fetch;
	DebugMemoryBusMaster busMaster = DebugMemoryBusMaster::Cpu;

	// The resolver's mapping epoch at resolution time. Transient: it orders
	// mapping changes within a session, it is never a stored identity.
	u64 mappingEpoch = 0;

	CDebugPhysicalAddress physical;

	// Byte offset in the exact loaded image file (a .crt, an iNES file with
	// header/trainer accounted for by the loader). Only when the loader knows.
	bool hasImageOffset = false;
	u64 imageOffset = 0;
};

class CDebugMemoryMappingResolver
{
public:
	CDebugMemoryMappingResolver();
	virtual ~CDebugMemoryMappingResolver();

	// Machine variant the mapping depends on, e.g. "C64 PAL", "Atari 130XE".
	virtual const char *GetMachineVariant() = 0;

	// Resolve one access. Implementations read the emulator's live banking
	// state; call it on the emulation thread or with the emulation paused.
	virtual CDebugMappedAddress Resolve(u16 cpuAddress, DebugMemoryAccess access,
										DebugMemoryBusMaster busMaster = DebugMemoryBusMaster::Cpu) = 0;

	CDebugMappedAddress ResolveFetch(u16 pc) { return Resolve(pc, DebugMemoryAccess::Fetch); }
	CDebugMappedAddress ResolveRead(u16 address) { return Resolve(address, DebugMemoryAccess::Read); }
	CDebugMappedAddress ResolveWrite(u16 address) { return Resolve(address, DebugMemoryAccess::Write); }

	// Every CPU address at which `physical` is currently visible for `access`:
	// none, one, or several aliases (mirrors, one bank in two windows). The
	// default scans the whole 64 KB space through Resolve(); a resolver with a
	// cheaper answer overrides it.
	virtual std::vector<u16> ResolveCpuWindows(const CDebugPhysicalAddress &physical,
											   DebugMemoryAccess access = DebugMemoryAccess::Fetch);

	// Incremented each time the resolver observes that the effective mapping
	// changed (see ObserveMappingState). Starts at 1 once state was observed.
	u64 GetMappingEpoch() const { return mappingEpoch; }

protected:
	// The cores send no "mapping changed" notification, so a resolver calls
	// this with a cheap signature of everything its mapping depends on (PLA
	// config + cartridge banks, mapper bank pointers, PORTB + cart state)
	// whenever it reads that state; the epoch advances when it differs from
	// the previous one.
	void ObserveMappingState(u64 stateSignature);

private:
	u64 mappingEpoch;
	u64 lastStateSignature;
	bool hasObservedState;
};

#endif
