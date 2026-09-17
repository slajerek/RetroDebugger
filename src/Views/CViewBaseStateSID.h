#ifndef _CViewBaseStateSID_H_
#define _CViewBaseStateSID_H_

#include "SYS_Defs.h"
#include "DebuggerDefs.h"
#include "CGuiView.h"
#include "CGuiEditHex.h"
#include "CGuiButtonSwitch.h"
#include "SYS_FileSystem.h"
#include "CRecentlyOpenedFiles.h"
#include <list>

class CSlrFont;
class CViewWaveform;
class CWaveformData;

// Number of SID chips the view can lay out at once. Shared with the C64
// debug interface so a VICE backend never reports more than there is room for.
#define SID_STATE_MAX_SIDS C64_MAX_NUM_SIDS

// The register file the view knows how to decode: $00..$1C, i.e. the 25
// writable registers plus POTX/POTY/OSC3/ENV3.
#define SID_STATE_NUM_REGISTERS 0x1D

// Generic SID state view.
//
// Holds all rendering, register editing, waveform binding, import/export and
// layout. Everything backend-specific goes through the virtuals below, so a
// subclass only supplies data access -- exactly the split CViewBaseStateCPU
// uses for CPU state views.
//
// Subclasses: CViewC64StateSID (VICE), CViewC64UStateSID (C64 Ultimate),
// CViewGT2StateSID (GoatTracker 2).
class CViewBaseStateSID : public CGuiView, CGuiEditHexCallback, CGuiButtonSwitchCallback,
						  CSystemFileDialogCallback, CRecentlyOpenedFilesCallback
{
public:
	// `recentsSettingsName` is the settings file the import/export recents list
	// is kept in. Each subclass passes its own: three SID views coexist, and a
	// shared file would mean each one's in-memory list clobbering the others'
	// on save.
	CViewBaseStateSID(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
					  const char *recentsSettingsName);
	virtual ~CViewBaseStateSID();

	//
	// Backend surface. This is the whole of what the view needs to exist.
	//
	virtual int GetNumSids() = 0;
	virtual u8 GetSidRegister(int sidNum, int registerNum) = 0;
	virtual void SetSidRegister(int sidNum, int registerNum, u8 value) = 0;
	// Base address used for the register address labels and the chip-select
	// button captions, e.g. 0xD400.
	virtual u16 GetSidBaseAddress(int sidNum) = 0;

	// How many registers this backend can actually report. GT2 keeps only
	// $00..$18 (it never reads OSC3/ENV3 back), the C64 exposes $00..$1C.
	virtual int GetNumRegisters();
	// Whether register editing is offered at all. C64 Ultimate's state cache
	// is a read-only mirror.
	virtual bool IsRegisterWritable();
	// Whether a write to this particular register survives longer than a
	// frame. Lets a backend whose player rewrites registers continuously mark
	// the ones it will immediately clobber.
	virtual bool IsRegisterWriteEffective(int sidNum, int registerNum);

	// Oscilloscope data. NULL for a backend that has none, in which case no
	// waveform is drawn and the layout closes up around it.
	virtual CWaveformData *GetChannelWaveform(int sidNum, int voice);
	virtual CWaveformData *GetMixWaveform(int sidNum);
	// Called after the view toggles a waveform's isMuted, so the backend can
	// push the new mute state into its mixer.
	virtual void UpdateWaveformsMuteStatus();
	// Called when the view becomes visible/invisible or the selected chip
	// changes, so a backend that only samples channel data on demand can
	// start and stop doing so.
	virtual void SetReceiveChannelsData(int sidNum, bool isReceiving);
	// Called after a register was edited through the UI.
	virtual void OnRegisterEdited();

	// Must be called at the end of the subclass constructor: it performs the
	// setup that depends on the virtuals above, which are not yet dispatched
	// to the subclass while the base constructor runs.
	void InitStateSID();

	//
	// CGuiView
	//
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool DoTap(float x, float y);

	virtual void SetVisible(bool isVisible);
	virtual void SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual void LayoutParameterChanged(CLayoutParameter *layoutParameter);

	virtual void Render();
	virtual void RenderImGui();
	virtual void DoLogic();
	virtual void RenderFocusBorder();

	virtual bool HasContextMenuItems();
	virtual void RenderContextMenuItems();

	virtual void Serialize(CByteBuffer *byteBuffer);
	virtual void Deserialize(CByteBuffer *byteBuffer);

	//
	// Rendering
	//
	CSlrFont *font;
	float fontScale;
	float fontHeight;

	CSlrFont *fontBytes;
	float fontBytesSize;

	bool showAllSidChips;
	bool showRegistersOnly;
	float oneSidStateSizeX;

	virtual void RenderStateSID(float posX, float posY, float posZ, CSlrFont *fontBytes, float fontSize);
	void PrintSidWaveform(uint8 wave, char *buf);

	//
	// Chip selection
	//
	int selectedSidNumber;
	CGuiButtonSwitch *btnsSelectSID[SID_STATE_MAX_SIDS];
	virtual bool ButtonSwitchChanged(CGuiButtonSwitch *button);
	float buttonSizeX;
	float buttonSizeY;

	void SelectSid(int sidNum);
	void UpdateSidButtonsState();
	void UpdateWaveformsPosition();

	// Cached backend configuration, so the chip buttons are rebuilt only when
	// the number of chips or their addresses actually change.
	int cachedNumSids;
	u16 cachedSidBaseAddress[SID_STATE_MAX_SIDS];

	//
	// Waveforms. Entries stay NULL when the backend supplies no data.
	//
	CViewWaveform *viewChannelWaveform[SID_STATE_MAX_SIDS][3];
	CViewWaveform *viewMixWaveform[SID_STATE_MAX_SIDS];
	bool HasWaveforms();

	//
	// Register editing
	//
	int editingRegisterValueIndex;		// -1 means no editing
	int editingSIDIndex;
	CGuiEditHex *editHex;
	virtual void GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled);

	//
	// Import / export of the raw register file
	//
	std::list<CSlrString *> sidRegsFileExtensions;
	virtual void SystemDialogFileOpenSelected(CSlrString *path);
	virtual void SystemDialogFileSaveSelected(CSlrString *path);

	CRecentlyOpenedFiles *recentlyOpened;
	virtual void RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath);

	virtual bool ImportSidRegs(CSlrString *path);
	virtual bool ExportSidRegs(CSlrString *path);
};

#endif
