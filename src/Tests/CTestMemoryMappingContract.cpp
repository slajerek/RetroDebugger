#include "CTestMemoryMappingContract.h"
#include "CDebugMemoryMapping.h"
#include <cstring>

// A small banked machine, shaped after the real ones:
//   $0000-$9FFF  RAM
//   $A000-$BFFF  ROM bank 0 or 1 for fetch/read when romVisible, else RAM;
//                writes always land in the RAM underneath (C64-style)
//   $C000-$CFFF  when mirror is on, the first 4 KB of the same ROM bank for
//                fetch/read, and writes go nowhere (Atari-style); else RAM
//   $D000-$DFFF  I/O
//   $E000-$FFFF  the resolver cannot tell: Unknown
class CFakeBankedResolver : public CDebugMemoryMappingResolver
{
public:
	bool romVisible = true;
	u32 romBank = 0;
	bool mirror = false;

	virtual const char *GetMachineVariant() override { return "Fake banked machine"; }

	virtual CDebugMappedAddress Resolve(u16 cpuAddress, DebugMemoryAccess access, DebugMemoryBusMaster busMaster) override
	{
		ObserveMappingState((romVisible ? 1 : 0) | (romBank << 1) | ((mirror ? 1 : 0) << 8));

		CDebugMappedAddress mapped;
		mapped.cpuAddress = cpuAddress;
		mapped.access = access;
		mapped.busMaster = busMaster;
		mapped.mappingEpoch = GetMappingEpoch();

		CDebugPhysicalAddress &p = mapped.physical;
		bool isWrite = (access == DebugMemoryAccess::Write);
		if (cpuAddress >= 0xA000 && cpuAddress <= 0xBFFF && romVisible && !isWrite)
		{
			SetRom(p, cpuAddress - 0xA000);
		}
		else if (cpuAddress >= 0xC000 && cpuAddress <= 0xCFFF && mirror && romVisible)
		{
			if (isWrite)
				p.kind = DebugMemoryKind::Unmapped;
			else
				SetRom(p, cpuAddress - 0xC000);
		}
		else if (cpuAddress >= 0xD000 && cpuAddress <= 0xDFFF)
		{
			p.kind = DebugMemoryKind::Io;
			p.device = "fake";
			p.offset = cpuAddress - 0xD000;
		}
		else if (cpuAddress >= 0xE000)
		{
			p.kind = DebugMemoryKind::Unknown;
		}
		else
		{
			p.kind = DebugMemoryKind::Ram;
			p.device = "fake";
			p.offset = cpuAddress;
		}
		return mapped;
	}

private:
	void SetRom(CDebugPhysicalAddress &p, u32 offset)
	{
		p.kind = DebugMemoryKind::Rom;
		p.device = "fake-rom";
		p.region = romBank;
		p.offset = offset;
	}
};

#define MM_CHECK(cond, msg) \
	do { \
		if (!(cond)) { TestCompleted(false, msg); return; } \
		StepCompleted(stepNum++, true, msg); \
	} while (0)

void CTestMemoryMappingContract::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;
	int stepNum = 1;

	CFakeBankedResolver resolver;
	MM_CHECK(resolver.GetMappingEpoch() == 0, "epoch is 0 before any state was observed");

	CDebugMappedAddress fetch = resolver.ResolveFetch(0xA000);
	CDebugMappedAddress write = resolver.ResolveWrite(0xA000);
	MM_CHECK(fetch.physical.kind == DebugMemoryKind::Rom && fetch.physical.region == 0 && fetch.physical.offset == 0
			 && write.physical.kind == DebugMemoryKind::Ram && write.physical.offset == 0xA000
			 && fetch.physical != write.physical,
			 "fetch and write at one address resolve to different storage");
	MM_CHECK(fetch.mappingEpoch == 1 && write.mappingEpoch == 1 && resolver.GetMappingEpoch() == 1,
			 "epoch is 1 after the first observation and stays while the state is unchanged");
	MM_CHECK(fetch.cpuAddress == 0xA000 && fetch.access == DebugMemoryAccess::Fetch
			 && fetch.busMaster == DebugMemoryBusMaster::Cpu && !fetch.hasImageOffset,
			 "a mapped address carries its CPU address, access kind and bus master");

	resolver.romBank = 1;
	CDebugMappedAddress fetchBank1 = resolver.ResolveFetch(0xA000);
	MM_CHECK(fetchBank1.physical.region == 1 && fetchBank1.physical != fetch.physical
			 && fetchBank1.mappingEpoch == 2,
			 "the same PC after a bank switch is a different identity, and the epoch advances");

	resolver.romVisible = false;
	CDebugMappedAddress fetchRam = resolver.ResolveFetch(0xA000);
	MM_CHECK(fetchRam.physical.kind == DebugMemoryKind::Ram && fetchRam.physical == write.physical
			 && fetchRam.mappingEpoch == 3,
			 "with the ROM banked out, fetch reaches the RAM the earlier write targeted");
	resolver.romVisible = true;

	CDebugMappedAddress unknown = resolver.ResolveFetch(0xE000);
	MM_CHECK(!unknown.physical.IsKnown() && unknown.physical != unknown.physical
			 && !(unknown.physical == resolver.ResolveFetch(0xE000).physical),
			 "Unknown is not a location and never equals anything, itself included");

	resolver.mirror = true;
	CDebugMappedAddress blocked = resolver.ResolveWrite(0xC000);
	MM_CHECK(blocked.physical.kind == DebugMemoryKind::Unmapped && !blocked.physical.IsKnown()
			 && resolver.ResolveRead(0xC000).physical.kind == DebugMemoryKind::Rom,
			 "a ROM window that blocks writes resolves the write as Unmapped");

	CDebugPhysicalAddress romByte = resolver.ResolveFetch(0xA010).physical;
	std::vector<u16> aliases = resolver.ResolveCpuWindows(romByte);
	MM_CHECK(aliases.size() == 2 && aliases[0] == 0xA010 && aliases[1] == 0xC010,
			 "ResolveCpuWindows returns every alias of a physical byte");
	resolver.mirror = false;
	aliases = resolver.ResolveCpuWindows(romByte);
	MM_CHECK(aliases.size() == 1 && aliases[0] == 0xA010, "one window once the mirror is off");
	resolver.romBank = 0;
	MM_CHECK(resolver.ResolveCpuWindows(romByte).empty(), "no window while that bank is not mapped");
	MM_CHECK(resolver.ResolveCpuWindows(unknown.physical).empty(), "an Unknown identity has no windows");

	CDebugPhysicalAddress ram = resolver.ResolveRead(0x1000).physical;
	MM_CHECK(ram.IsMutable() && !romByte.IsMutable() && !resolver.ResolveRead(0xD000).physical.IsMutable(),
			 "RAM is mutable, ROM and I/O are not");

	MM_CHECK(strcmp(DebugMemoryKindName(DebugMemoryKind::CartridgeRom), "Cartridge ROM") == 0
			 && strcmp(DebugMemoryAccessName(DebugMemoryAccess::Write), "write") == 0,
			 "kinds and access kinds have printable names");

	TestCompleted(true, "logical vs physical identity contract holds");
}
