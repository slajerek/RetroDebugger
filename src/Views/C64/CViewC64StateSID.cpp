#include "CViewC64StateSID.h"

#include "SYS_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "CSlrFileFromOS.h"
#include "CViewC64.h"
#include "CDebugInterfaceC64.h"
#include "CDebugInterfaceVice.h"
#include "CViewC64SidTrackerHistory.h"
#include "CWaveformData.h"
#include "C64Tools.h"
#include "CByteBuffer.h"

CViewC64StateSID::CViewC64StateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
// Keeps the historical settings name, so a user's existing recents list survives.
: CViewBaseStateSID(name, posX, posY, posZ, sizeX, sizeY, "recents-sidregs")
{
	this->debugInterface = debugInterface;

	// local sid data for store/restore purposes
	sidData = new CSidData();

	InitStateSID();
}

CViewC64StateSID::~CViewC64StateSID()
{
}

int CViewC64StateSID::GetNumSids()
{
	return debugInterface->GetNumSids();
}

u8 CViewC64StateSID::GetSidRegister(int sidNum, int registerNum)
{
	return debugInterface->GetSidRegister(sidNum, registerNum);
}

void CViewC64StateSID::SetSidRegister(int sidNum, int registerNum, u8 value)
{
	debugInterface->SetSidRegister(sidNum, registerNum, value);
}

u16 CViewC64StateSID::GetSidBaseAddress(int sidNum)
{
	return GetSidAddressByChipNum(sidNum);
}

CWaveformData *CViewC64StateSID::GetChannelWaveform(int sidNum, int voice)
{
	return debugInterface->sidChannelWaveform[sidNum][voice];
}

CWaveformData *CViewC64StateSID::GetMixWaveform(int sidNum)
{
	return debugInterface->sidMixWaveform[sidNum];
}

void CViewC64StateSID::UpdateWaveformsMuteStatus()
{
	debugInterface->UpdateWaveformsMuteStatus();
}

void CViewC64StateSID::SetReceiveChannelsData(int sidNum, bool isReceiving)
{
	debugInterface->SetSIDReceiveChannelsData(sidNum, isReceiving);
}

void CViewC64StateSID::OnRegisterEdited()
{
	viewC64->viewC64SidTrackerHistory->UpdateHistoryWithCurrentSidData();
}

bool CViewC64StateSID::ImportSidRegs(CSlrString *path)
{
	CSlrFile *file = new CSlrFileFromOS(path);
	if (!file->Exists())
	{
		guiMain->ShowMessageBox("Error", "Import SID registers failed. Can't open file.");
		delete file;
		return false;
	}

	CByteBuffer *byteBuffer = new CByteBuffer(file);

	sidData->Deserialize(byteBuffer);
	sidData->RestoreSids();

	delete byteBuffer;
	delete file;

	return true;
}

bool CViewC64StateSID::ExportSidRegs(CSlrString *path)
{
	LOGM("CViewC64StateSID::ExportSidRegs");

	CByteBuffer *byteBuffer = new CByteBuffer();
	sidData->PeekFromSids();
	sidData->Serialize(byteBuffer);
	byteBuffer->storeToFile(path);
	delete byteBuffer;

	return true;
}
