#ifndef _CVIEWC64STATESID_H_
#define _CVIEWC64STATESID_H_

#include "SYS_Defs.h"
#include "EmulatorsConfig.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include "ViceWrapper.h"
#include "CGuiButtonSwitch.h"
#include "SYS_FileSystem.h"
#include "CRecentlyOpenedFiles.h"
#include <vector>
#include <list>

class CSlrFont;
class CDataAdapter;
class CViewDataMap;
class CSlrMutex;
class CDebugInterfaceC64;
class CViewWaveform;
class CSidData;

// TODO: make base class to have Vice specific state rendering and editing
//       class CViewC64ViceStateSID : public CViewC64StateSID

class CViewC64StateSID : public CGuiView, CGuiEditHexCallback, CGuiButtonSwitchCallback, CSystemFileDialogCallback, CRecentlyOpenedFilesCallback
{
public:
	CViewC64StateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);

	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual bool DoTap(float x, float y);
	
	CSlrFont *font;
	float fontScale;
	float fontHeight;
	
	CSlrFont *fontBytes;
	float fontBytesSize;
	
	virtual void SetVisible(bool isVisible);
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	
	bool showAllSidChips;
	
	CDebugInterfaceC64 *debugInterface;
	
	void DumpSidWaveform(uint8 wave, char *buf);
	
	// [sid num][channel num]
	CViewWaveform *viewChannelWaveform[C64_MAX_NUM_SIDS][3];
	CViewWaveform *viewMixWaveform[C64_MAX_NUM_SIDS];
	
	int selectedSidNumber;
	
	CGuiButtonSwitch *btnsSelectSID[C64_MAX_NUM_SIDS];
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);
	float buttonSizeX;
	float buttonSizeY;

	void SelectSid(int sidNum);
	
	virtual void RenderStateSID(int sidNum, float posX, float posY, float posZ, CSlrFont *fontBytes, float fontSize);
	void PrintSidWaveform(uint8 wave, char *buf);
	
	void UpdateSidButtonsState();
	void UpdateWaveformsPosition();
	
	// editing registers
	bool showRegistersOnly;
	int editingRegisterValueIndex;		// -1 means no editing
	int editingSIDIndex;
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);

	float oneSidStateSizeX;
	
	//
	virtual void RenderFocusBorder();

	std::list<CSlrString *> sidRegsFileExtensions;
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileSaveSelected(CSlrString *path);

	CRecentlyOpenedFiles *recentlyOpened;
	virtual void RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath);

	// returns charset addr
	CSidData *sidData;
	bool ImportSidRegs(CSlrString *path);
	bool ExportSidRegs(CSlrString *path);

	//
	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	// Layout
	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

};


#endif

