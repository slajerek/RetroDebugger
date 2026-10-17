#ifndef _CNesMemoryMappingResolver_h_
#define _CNesMemoryMappingResolver_h_

#include "CDebugMemoryMapping.h"

// The NES memory mapping provider (RD-040): CPU accesses resolved through the
// cartridge board's own page tables (nesd_get_prg_mapping), so the mapper is
// the authority for which PRG bank sits in which window.
//
// Physical identities ("device", region, offset):
//   nes-ram 0 (2 KB internal RAM; $0800-$1FFF are aliases of it),
//   nes-ppu 0 (register 0-7, mirrored through $3FFF), nes-apu-io 0 ($4000-$401F),
//   nes-prg <source> (source 0 = PRG-ROM, byte offset in it),
//   nes-wram <source> ($6000-$7FFF work RAM),
//   nes-mapper 0 (a write to PRG-ROM space reaches the mapper's registers, offset
//   from $8000).
// $4020-$5FFF (expansion area) and boards that answer windows with their own
// handlers (MMC5 and similar) are Unknown. The iNES file offset is not given:
// the cartridge profile does not keep the header's trainer flag.
class CNesMemoryMappingResolver : public CDebugMemoryMappingResolver
{
public:
	virtual const char *GetMachineVariant() override { return "NES (Nestopia)"; }
	virtual CDebugMappedAddress Resolve(u16 cpuAddress, DebugMemoryAccess access,
										DebugMemoryBusMaster busMaster = DebugMemoryBusMaster::Cpu) override;
};

#endif
