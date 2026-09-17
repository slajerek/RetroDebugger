#include "CViewC64UStateSID.h"

#include "../../Emulators/c64u/CDebugInterfaceC64U.h"
#include "../../Emulators/c64u/State/C64ULogicalStateCache.h"

CViewC64UStateSID::CViewC64UStateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64U *debugInterface)
: CViewBaseStateSID(name, posX, posY, posZ, sizeX, sizeY, "recents-sidregs-c64u")
{
	this->debugInterface = debugInterface;

	InitStateSID();
}

int CViewC64UStateSID::GetNumSids()
{
	return 1;
}

u8 CViewC64UStateSID::GetSidRegister(int sidNum, int registerNum)
{
	if (registerNum < 0 || registerNum >= 0x20)
		return 0;

	C64ULogicalStateCache *stateCache = debugInterface->GetLogicalStateCache();
	if (stateCache == NULL)
		return 0;

	C64USidState sid = stateCache->GetSidState();
	return sid.registers[registerNum];
}

void CViewC64UStateSID::SetSidRegister(int sidNum, int registerNum, u8 value)
{
	// The Ultimate's state cache is a read-only mirror.
}

u16 CViewC64UStateSID::GetSidBaseAddress(int sidNum)
{
	return 0xD400;
}

int CViewC64UStateSID::GetNumRegisters()
{
	// The cache mirrors the whole $D400-$D41F window.
	return 0x20;
}

bool CViewC64UStateSID::IsRegisterWritable()
{
	return false;
}
