#ifndef _CAtariMemoryMappingResolver_h_
#define _CAtariMemoryMappingResolver_h_

#include "CDebugMemoryMapping.h"

// The Atari 8-bit memory mapping provider (RD-029): RAM/ROM/hardware per
// address comes from Atari800's own attribute map; which ROM, which extended
// bank and which cartridge bank from its banking state (atrd_get_banking_state),
// so PORTB decoding, machine model and RAM size are the emulator's answer.
//
// Physical identities ("device", region, offset):
//   atari-ram 0 (main RAM, offset = address), atari-xe-ram <bank> (130XE-style
//   bank at $4000-$7FFF, bank 1..n, offset from $4000), atari-os 0 (offset in
//   the OS image as Atari800 holds it: XL/XE $C000-$FFFF with self-test at
//   $1000; 400/800 $D800-$FFFF), atari-basic 0, atari-xegame 0, atari-io 0
//   (offset from $D000), atari-cart <bank> (standard 8K/16K only).
// Writes to ROM are dropped (Unmapped). Other cartridge types and the 5200
// are Unknown. Bus master ANTIC sees what the CPU reads, except in the
// extended RAM window, which follows ANTIC's own bank.
class CAtariMemoryMappingResolver : public CDebugMemoryMappingResolver
{
public:
	virtual const char *GetMachineVariant() override;
	virtual CDebugMappedAddress Resolve(u16 cpuAddress, DebugMemoryAccess access,
										DebugMemoryBusMaster busMaster = DebugMemoryBusMaster::Cpu) override;
	// RD-059: base and extended RAM, the OS/BASIC/XEGS ROMs and the cartridge
	// image from Atari800's arrays (atrd_peek_physical); I/O is not read
	virtual bool ReadPhysical(const CDebugPhysicalAddress &physical, u8 *value) override;
private:
	char variant[64];
};

#endif
