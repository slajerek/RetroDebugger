#include "CViceMemoryMappingResolver.h"

extern "C"
{
#include "ViceWrapper.h"
}

CDebugMappedAddress CViceMemoryMappingResolver::Resolve(u16 cpuAddress, DebugMemoryAccess access,
														DebugMemoryBusMaster busMaster)
{
	int memConfig = 0, cartType = 0, exrom = 0, game = 0, romlBank = 0, romhBank = 0, exportRam = 0;
	c64d_get_banking_state(&memConfig, &cartType, &exrom, &game, &romlBank, &romhBank, &exportRam);
	ObserveMappingState((u64)(u32)memConfig | ((u64)(u8)cartType << 8) | ((u64)(u16)romlBank << 16)
						| ((u64)(u16)romhBank << 32) | ((u64)(exportRam & 1) << 48));

	CDebugMappedAddress mapped;
	mapped.cpuAddress = cpuAddress;
	mapped.access = access;
	mapped.busMaster = busMaster;
	mapped.mappingEpoch = GetMappingEpoch();

	if (busMaster != DebugMemoryBusMaster::Cpu)
		return mapped;	// Unknown: the VIC-II view is not modelled yet

	int kind = C64D_ACCESS_UNKNOWN;
	uint32 offset = 0;
	c64d_resolve_access(cpuAddress, access == DebugMemoryAccess::Write ? 1 : 0, &kind, &offset);

	CDebugPhysicalAddress &p = mapped.physical;
	p.offset = offset;
	switch (kind)
	{
		case C64D_ACCESS_RAM:		p.kind = DebugMemoryKind::Ram;			p.device = "c64-ram";		break;
		case C64D_ACCESS_CPU_PORT:	p.kind = DebugMemoryKind::CpuPort;		p.device = "c64-cpu-port";	break;
		case C64D_ACCESS_BASIC:		p.kind = DebugMemoryKind::Rom;			p.device = "c64-basic";		break;
		case C64D_ACCESS_KERNAL:	p.kind = DebugMemoryKind::Rom;			p.device = "c64-kernal";	break;
		case C64D_ACCESS_CHARGEN:	p.kind = DebugMemoryKind::Rom;			p.device = "c64-chargen";	break;
		case C64D_ACCESS_IO:		p.kind = DebugMemoryKind::Io;			p.device = "c64-io";		break;
		case C64D_ACCESS_COLORRAM:	p.kind = DebugMemoryKind::Ram;			p.device = "c64-color-ram";	break;
		case C64D_ACCESS_ROML:
		{
			p.kind = DebugMemoryKind::CartridgeRom;	p.device = "c64-cart";		p.region = 0;
			uint32 fileOffset = 0;
			if (c64d_cartridge_roml_file_offset(offset, &fileOffset))
			{
				mapped.hasImageOffset = true;	// the byte's offset in the attached .crt (RD-030)
				mapped.imageOffset = fileOffset;
			}
			break;
		}
		case C64D_ACCESS_ROMH:		p.kind = DebugMemoryKind::CartridgeRom;	p.device = "c64-cart";		p.region = 1; break;
		case C64D_ACCESS_CART_RAM:	p.kind = DebugMemoryKind::CartridgeRam;	p.device = "c64-cart-ram";	break;
		case C64D_ACCESS_UNMAPPED:	p.kind = DebugMemoryKind::Unmapped;		p.offset = 0;				break;
		default:					p.kind = DebugMemoryKind::Unknown;		p.offset = 0;				break;
	}
	return mapped;
}
