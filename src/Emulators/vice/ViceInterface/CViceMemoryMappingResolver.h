#ifndef _CViceMemoryMappingResolver_h_
#define _CViceMemoryMappingResolver_h_

#include "CDebugMemoryMapping.h"

// The C64 memory mapping provider (RD-028): resolves CPU accesses through
// VICE's own memory configuration tables (c64d_resolve_access), so the 6510
// port, the PLA, cartridge modes and Ultimax are VICE's answer.
//
// Physical identities ("device", region, offset):
//   c64-ram 0 (64 KB RAM, incl. RAM under ROM), c64-cpu-port 0 ($00/$01),
//   c64-basic 0, c64-kernal 0, c64-chargen 0, c64-io 0 (offset from $D000),
//   c64-color-ram 0, c64-cart 0 (ROML image, bank * 8 KB + offset),
//   c64-cart 1 (ROMH image), c64-cart-ram 0.
// Cartridge windows are exact only for the generic 8K/16K/Ultimax types with
// nothing in VICE's slots 0/1; other cartridge types answer Unknown there
// until their own profiles exist (spec #3B.2). Fetch and read route alike on
// the 6510. The VIC-II's view is not modelled yet: bus master Vic is Unknown.
class CViceMemoryMappingResolver : public CDebugMemoryMappingResolver
{
public:
	virtual const char *GetMachineVariant() override { return "C64 (VICE)"; }
	virtual CDebugMappedAddress Resolve(u16 cpuAddress, DebugMemoryAccess access,
										DebugMemoryBusMaster busMaster = DebugMemoryBusMaster::Cpu) override;
};

#endif
