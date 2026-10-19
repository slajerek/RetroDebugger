#include "CAtariMemoryMappingResolver.h"
#include "AtariWrapper.h"
#include <cstdio>

// Atari800 values (atari.h, memory.h, a-cartridge.h), mirrored to keep the
// emulator headers out of this file.
static const int kMachine800 = 0;
static const int kMachineXLXE = 1;
static const int kAttribRam = 0;
static const int kAttribRom = 1;
static const int kAttribHardware = 2;
static const int kCartStd8 = 1;
static const int kCartStd16 = 2;
static const int kCartXegs32 = 12;
static const int kCartSwXegs32 = 33;

// a-cartridge.c set_bank_809F: an 8 KB bank at $8000 (state), the last bank
// at $A000; state bit 7 switches the cartridge off (the switchable ones)
static bool IsXegs(int cartType)
{
	return cartType == kCartXegs32 || cartType == 13 /* XEGS_07_64 */ || cartType == 14 /* XEGS_128 */
		|| (cartType >= 23 && cartType <= 25) /* XEGS_256..1024 */ || (cartType >= kCartSwXegs32 && cartType <= 38) /* SWXEGS_32..1024 */;
}

// a-cartridge.c set_bank_80BF: one 16 KB bank at $8000-$BFFF (state & $7F)
static bool IsMegaCart(int cartType)
{
	return (cartType >= 26 && cartType <= 32) /* MEGA_16..1024 */ || cartType == 64 /* MEGA_2048 */;
}

const char *CAtariMemoryMappingResolver::GetMachineVariant()
{
	int machineType = 0, ramSizeKb = 0;
	atrd_get_banking_state(&machineType, &ramSizeKb, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	snprintf(variant, sizeof(variant), "Atari %s %d KB",
			 machineType == kMachine800 ? "400/800" : (machineType == kMachineXLXE ? "XL/XE" : "5200"), ramSizeKb);
	return variant;
}

CDebugMappedAddress CAtariMemoryMappingResolver::Resolve(u16 cpuAddress, DebugMemoryAccess access,
														 DebugMemoryBusMaster busMaster)
{
	int machineType = 0, ramSizeKb = 0, portb = 0, portbMask = 0, xeBank = 0, anticXeBank = 0;
	int selftest = 0, cart809f = 0, cartA0bf = 0, cartType = 0, cartState = 0, cartSizeKb = 0;
	atrd_get_banking_state(&machineType, &ramSizeKb, &portb, &portbMask, &xeBank, &anticXeBank,
						   &selftest, &cart809f, &cartA0bf, &cartType, &cartState, &cartSizeKb);
	int builtinA000 = atrd_get_builtin_rom_a000();
	ObserveMappingState((u64)(portb | portbMask) | ((u64)xeBank << 8) | ((u64)anticXeBank << 16)
						| ((u64)(selftest & 1) << 24) | ((u64)(cart809f & 1) << 25) | ((u64)(cartA0bf & 1) << 26)
						| ((u64)(builtinA000 & 3) << 27) | ((u64)(u16)cartState << 32) | ((u64)(u8)cartType << 48));

	CDebugMappedAddress mapped;
	mapped.cpuAddress = cpuAddress;
	mapped.access = access;
	mapped.busMaster = busMaster;
	mapped.mappingEpoch = GetMappingEpoch();
	CDebugPhysicalAddress &p = mapped.physical;

	bool isAntic = (busMaster == DebugMemoryBusMaster::Antic);
	if (busMaster != DebugMemoryBusMaster::Cpu && !isAntic)
		return mapped;
	if (machineType != kMachine800 && machineType != kMachineXLXE)
		return mapped;	// the 5200 has its own map: Unknown

	// The 130XE-style window: CPU and ANTIC can see different banks
	int bank = isAntic ? anticXeBank : xeBank;
	if (cpuAddress >= 0x4000 && cpuAddress < 0x8000 && bank > 0 && !(selftest && cpuAddress >= 0x5000 && cpuAddress < 0x5800))
	{
		p.kind = DebugMemoryKind::ExtendedRam;
		p.device = "atari-xe-ram";
		p.region = (u32)bank;
		p.offset = cpuAddress - 0x4000;
		return mapped;
	}

	int attrib = atrd_get_memory_attrib(cpuAddress);
	if (attrib == kAttribRam)
	{
		p.kind = DebugMemoryKind::Ram;
		p.device = "atari-ram";
		p.offset = cpuAddress;
	}
	else if (attrib == kAttribHardware)
	{
		p.kind = DebugMemoryKind::Io;
		p.device = "atari-io";
		p.offset = cpuAddress - 0xD000;
	}
	else if (attrib == kAttribRom)
	{
		if (access == DebugMemoryAccess::Write && !isAntic)
		{
			p.kind = DebugMemoryKind::Unmapped;	// Atari ROM drops writes
		}
		else if ((cpuAddress >= 0xA000 && cpuAddress < 0xC000 && cartA0bf)
				 || (cpuAddress >= 0x8000 && cpuAddress < 0xA000 && cart809f))
		{
			bool known = true;
			if (cartType == kCartStd8 && cpuAddress >= 0xA000)
				p.offset = cpuAddress - 0xA000;
			else if (cartType == kCartStd16)
				p.offset = cpuAddress - 0x8000;
			else if (IsXegs(cartType) && !(cartState & 0x80) && cartSizeKb >= 32)
			{
				// $8000 is bank `state` (masked to the size when written), $A000 the last bank
				p.offset = cpuAddress >= 0xA000 ? (u32)cartSizeKb * 1024 - 0x2000 + (cpuAddress - 0xA000)
												: (u32)cartState * 0x2000 + (cpuAddress - 0x8000);
			}
			else if (IsMegaCart(cartType) && !(cartState & 0x80))
			{
				p.offset = (u32)(cartState & 0x7F) * 0x4000 + (cpuAddress - 0x8000);
			}
			else
				known = false;	// other cartridge types bank on their own: Unknown until profiled
			if (known)
			{
				p.kind = DebugMemoryKind::CartridgeRom;
				p.device = "atari-cart";
				mapped.hasImageOffset = true;		// the byte's offset in the cartridge file (RD-030)
				mapped.imageOffset = (u64)atrd_get_cart_file_header_size() + p.offset;
			}
		}
		else if (cpuAddress >= 0xA000 && cpuAddress < 0xC000 && builtinA000 != 0)
		{
			p.kind = DebugMemoryKind::Rom;
			p.device = builtinA000 == 1 ? "atari-basic" : "atari-xegame";
			p.offset = cpuAddress - 0xA000;
		}
		else if (machineType == kMachineXLXE && selftest && cpuAddress >= 0x5000 && cpuAddress < 0x5800)
		{
			p.kind = DebugMemoryKind::Rom;
			p.device = "atari-os";
			p.offset = 0x1000 + (cpuAddress - 0x5000);	// the self-test is the OS's $D000-$D7FF
		}
		else if (machineType == kMachineXLXE && cpuAddress >= 0xC000)
		{
			p.kind = DebugMemoryKind::Rom;
			p.device = "atari-os";
			p.offset = cpuAddress - 0xC000;
		}
		else if (machineType == kMachine800 && cpuAddress >= 0xD800)
		{
			p.kind = DebugMemoryKind::Rom;
			p.device = "atari-os";
			p.offset = cpuAddress - 0xD800;
		}
	}
	return mapped;
}

bool CAtariMemoryMappingResolver::ReadPhysical(const CDebugPhysicalAddress &physical, u8 *value)
{
	const std::string &d = physical.device;
	int device = d == "atari-ram" ? 0 : d == "atari-xe-ram" ? 1 : d == "atari-os" ? 2
		: d == "atari-basic" ? 3 : d == "atari-xegame" ? 4 : d == "atari-cart" ? 5 : -1;
	if (device < 0)
		return false;
	int v = atrd_peek_physical(device, (int)physical.region, (int)physical.offset);
	if (v < 0)
		return false;
	*value = (u8)v;
	return true;
}
