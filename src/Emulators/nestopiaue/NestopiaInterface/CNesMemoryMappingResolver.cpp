#include "CNesMemoryMappingResolver.h"
#include "NesWrapper.h"

CDebugMappedAddress CNesMemoryMappingResolver::Resolve(u16 cpuAddress, DebugMemoryAccess access,
													   DebugMemoryBusMaster busMaster)
{
	// The mapping state: where each 8 KB window of $6000-$FFFF points right now.
	u64 signature = 0;
	for (u32 window = 0x6000; window <= 0xE000; window += 0x2000)
	{
		int source = -1;
		unsigned int offset = 0;
		if (nesd_get_prg_mapping((unsigned short)window, NULL, &source, &offset, NULL))
			signature = signature * 1000003ULL + ((u64)(source + 1) << 32) + offset;
		else
			signature = signature * 1000003ULL + 0xFFFFFFFFULL;
	}
	ObserveMappingState(signature);

	CDebugMappedAddress mapped;
	mapped.cpuAddress = cpuAddress;
	mapped.access = access;
	mapped.busMaster = busMaster;
	mapped.mappingEpoch = GetMappingEpoch();
	if (busMaster != DebugMemoryBusMaster::Cpu)
		return mapped;	// Unknown: the PPU's address space is not modelled here

	CDebugPhysicalAddress &p = mapped.physical;
	bool isWrite = (access == DebugMemoryAccess::Write);

	if (cpuAddress < 0x2000)
	{
		p.kind = DebugMemoryKind::Ram;
		p.device = "nes-ram";
		p.offset = cpuAddress & 0x07FF;
	}
	else if (cpuAddress < 0x4000)
	{
		p.kind = DebugMemoryKind::Io;
		p.device = "nes-ppu";
		p.offset = cpuAddress & 0x0007;
	}
	else if (cpuAddress < 0x4020)
	{
		p.kind = DebugMemoryKind::Io;
		p.device = "nes-apu-io";
		p.offset = cpuAddress - 0x4000;
	}
	else if (cpuAddress >= 0x6000)
	{
		int isWrk = 0, source = 0, writable = 0;
		unsigned int offset = 0;
		if (!nesd_get_prg_mapping(cpuAddress, &isWrk, &source, &offset, &writable))
		{
			// a loaded board with nothing at $6000 answers open bus; no cartridge: unknown
			int mapper = 0;
			if (isWrk || cpuAddress < 0x8000)
				p.kind = nesd_get_cartridge_info(&mapper, NULL, NULL) ? DebugMemoryKind::Unmapped : DebugMemoryKind::Unknown;
		}
		else if (isWrite && !writable)
		{
			// writes to ROM space go to the mapper's registers, not to storage
			p.kind = DebugMemoryKind::Io;
			p.device = "nes-mapper";
			p.offset = cpuAddress - 0x8000;
		}
		else if (isWrk)
		{
			p.kind = DebugMemoryKind::CartridgeRam;
			p.device = "nes-wram";
			p.region = (u32)source;
			p.offset = offset;
		}
		else
		{
			p.kind = writable ? DebugMemoryKind::CartridgeRam : DebugMemoryKind::CartridgeRom;
			p.device = "nes-prg";
			p.region = (u32)source;
			p.offset = offset;
			// PRG-ROM bytes come from the image file at a known offset (RD-007)
			unsigned long long prgStart = 0;
			if (source == 0 && !writable && nesd_get_prg_image_start(&prgStart))
			{
				mapped.hasImageOffset = true;
				mapped.imageOffset = prgStart + offset;
			}
		}
	}
	return mapped;
}
