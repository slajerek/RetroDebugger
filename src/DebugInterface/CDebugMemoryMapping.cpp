#include "CDebugMemoryMapping.h"

const char *DebugMemoryKindName(DebugMemoryKind kind)
{
	switch (kind)
	{
		case DebugMemoryKind::Unknown:		return "Unknown";
		case DebugMemoryKind::Unmapped:		return "Unmapped";
		case DebugMemoryKind::Ram:			return "RAM";
		case DebugMemoryKind::Rom:			return "ROM";
		case DebugMemoryKind::Io:			return "I/O";
		case DebugMemoryKind::CpuPort:		return "CPU port";
		case DebugMemoryKind::CartridgeRom:	return "Cartridge ROM";
		case DebugMemoryKind::CartridgeRam:	return "Cartridge RAM";
		case DebugMemoryKind::ExtendedRam:	return "Extended RAM";
	}
	return "?";
}

const char *DebugMemoryAccessName(DebugMemoryAccess access)
{
	switch (access)
	{
		case DebugMemoryAccess::Fetch:	return "fetch";
		case DebugMemoryAccess::Read:	return "read";
		case DebugMemoryAccess::Write:	return "write";
	}
	return "?";
}

bool CDebugPhysicalAddress::IsMutable() const
{
	return kind == DebugMemoryKind::Ram || kind == DebugMemoryKind::CartridgeRam
		|| kind == DebugMemoryKind::ExtendedRam;
}

bool CDebugPhysicalAddress::operator==(const CDebugPhysicalAddress &other) const
{
	// Unknown is never equal to anything, itself included: two unresolved
	// accesses are not proven to be the same byte.
	if (kind == DebugMemoryKind::Unknown || other.kind == DebugMemoryKind::Unknown)
		return false;
	return kind == other.kind && region == other.region && offset == other.offset && device == other.device;
}

CDebugMemoryMappingResolver::CDebugMemoryMappingResolver()
{
	mappingEpoch = 0;
	lastStateSignature = 0;
	hasObservedState = false;
}

CDebugMemoryMappingResolver::~CDebugMemoryMappingResolver()
{
}

std::vector<u16> CDebugMemoryMappingResolver::ResolveCpuWindows(const CDebugPhysicalAddress &physical,
																 DebugMemoryAccess access)
{
	std::vector<u16> windows;
	if (!physical.IsKnown())
		return windows;

	for (u32 address = 0; address <= 0xFFFF; address++)
	{
		CDebugMappedAddress mapped = Resolve((u16)address, access);
		if (mapped.physical == physical)
			windows.push_back((u16)address);
	}
	return windows;
}

void CDebugMemoryMappingResolver::ObserveMappingState(u64 stateSignature)
{
	if (!hasObservedState || stateSignature != lastStateSignature)
	{
		hasObservedState = true;
		lastStateSignature = stateSignature;
		mappingEpoch++;
	}
}
