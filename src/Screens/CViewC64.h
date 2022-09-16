/*
 *  C64 Debugger
 *
 *  Created by Marcin Skoczylas on 16-02-22.
 *  Copyright 2016 Marcin Skoczylas. All rights reserved.
 *
 */

#ifndef _GUI_C64_
#define _GUI_C64_

#include "C64D_Version.h"
#include "CGuiView.h"
#include "CGuiButton.h"
#include "SYS_Threading.h"
#include "SYS_Defs.h"
#include "SYS_PauseResume.h"
#include "CViewMainMenu.h"
#include "CViewSettingsMenu.h"
#include "SYS_SharedMemory.h"
#include "CGuiViewSaveFile.h"
#include "CGuiViewSelectFile.h"
#include "CSlrKeyboardShortcuts.h"
#include "CConfigStorageHjson.h"
#include "CGlobalDropFileCallback.h"
#include "CRecentlyOpenedFiles.h"
#include "CGuiViewProgressBarWindow.h"

extern "C"
{
#include "ViceWrapper.h"
};

#include <list>
#include <vector>
#include <map>

class ImFont;

class CDebugInterface;
class CDebugInterfaceC64;
class CDebugInterfaceAtari;
class CDebugInterfaceNes;

class CDebuggerEmulatorPlugin;

class C64KeyboardShortcuts;
class CSlrFontProportional;
class CSlrKeyboardShortcut;
class CSlrKeyboardShortcuts;
class CC64DataAdapter;
class CC64DirectRamDataAdapter;
class CC64DiskDataAdapter;
class CC64DiskDirectRamDataAdapter;
class CDebugSymbols;

class CGuiViewMessages;

class CViewC64Screen;
class CViewC64ScreenWrapper;
class CViewC64ScreenViewfinder;

class CViewMemoryMap;
class CViewDataDump;
class CViewDataWatch;
class CViewBreakpoints;
class CViewDisassembly;
class CViewSourceCode;
class CViewC64StateCPU;
class CViewC64StateCIA;
class CViewC64StateSID;
class CViewC64StateVIC;
class CViewC64VicDisplay;
class CViewC64VicControl;
class CViewC64SidTrackerHistory;
class CViewC64SidPianoKeyboard;
class CViewC64MemoryDebuggerLayoutToolbar;
class CViewVicEditor;
class CViewDrive1541StateCPU;
class CViewDrive1541StateVIA;
class CViewDrive1541FileD64;
class CViewC64StateREU;
class CViewC64AllGraphics;
class CViewEmulationState;
class CViewEmulationCounters;
class CViewTimeline;
class CViewInputEvents;
class CViewMonitorConsole;
class CGuiViewUiDebug;

class CViewAtariScreen;
class CViewAtariStateCPU;
class CViewAtariStateANTIC;
class CViewAtariStatePIA;
class CViewAtariStateGTIA;
class CViewAtariStatePOKEY;

class CViewNesScreen;
class CViewNesStateCPU;
class CViewNesStateAPU;
class CViewNesStatePPU;
class CViewNesPpuPatterns;
class CViewNesPpuNametables;
class CViewNesPpuAttributes;
class CViewNesPpuOam;
class CViewNesPpuPalette;
class CViewNesPianoKeyboard;

class CViewJukeboxPlaylist;
class CViewMainMenu;
class CViewSettingsMenu;
class CViewDrive1541FileD64;
class CViewC64KeyMap;
class CViewKeyboardShortcuts;
class CViewSnapshots;
class CViewColodore;
class CViewAbout;

class CMainMenuBar;

class CColorsTheme;

class CEmulationThreadC64 : public CSlrThread
{
	void ThreadRun(void *data);
};

class CEmulationThreadAtari : public CSlrThread
{
	void ThreadRun(void *data);
};

class CEmulationThreadNes : public CSlrThread
{
	void ThreadRun(void *data);
};


class CViewC64 : public CGuiView, CGuiButtonCallback, CApplicationPauseResumeListener,
				 public CSharedMemorySignalCallback, public CGuiViewSelectFileCallback, public CGuiViewSaveFileCallback,
				 public CSlrKeyboardShortcutCallback, public CGlobalDropFileCallback, public CRecentlyOpenedFilesCallback
{
public:
	CViewC64(float posX, float posY, float posZ, float sizeX, float sizeY);
	virtual ~CViewC64();

	virtual void Render();
	virtual void Render(float posX, float posY);
	//virtual void Render(float posX, float posY, float sizeX, float sizeY);
	virtual void DoLogic();

	virtual void RenderImGui();

	virtual bool DoTap(float x, float y);
	virtual bool DoFinishTap(float x, float y);

	virtual bool DoDoubleTap(float x, float y);
	virtual bool DoFinishDoubleTap(float posX, float posY);

	virtual bool DoRightClick(float x, float y);
	virtual bool DoFinishRightClick(float x, float y);

	virtual bool DoMove(float x, float y, float distX, float distY, float diffX, float diffY);
	virtual bool FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY);

	virtual bool InitZoom();
	virtual bool DoZoomBy(float x, float y, float zoomValue, float difference);
	
	// multi touch
	virtual bool DoMultiTap(COneTouchData *touch, float x, float y);
	virtual bool DoMultiMove(COneTouchData *touch, float x, float y);
	virtual bool DoMultiFinishTap(COneTouchData *touch, float x, float y);

	virtual void FinishTouches();

	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	virtual bool KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);	// repeats
	
	virtual bool DoScrollWheel(float deltaX, float deltaY);
	virtual bool DoNotTouchedMove(float x, float y);

	virtual void ActivateView();
	virtual void DeactivateView();

	volatile bool isInitialized;

	// Note: this is new hjson-style config that will replace existing CConfigStorage
	CConfigStorageHjson *config;

	std::vector<CDebugInterface *> debugInterfaces;
	
	CDebugInterfaceC64 *debugInterfaceC64;
	CEmulationThreadC64 *emulationThreadC64;

	CDebugInterfaceAtari *debugInterfaceAtari;
	CEmulationThreadAtari *emulationThreadAtari;

	CDebugInterfaceNes *debugInterfaceNes;
	CEmulationThreadNes *emulationThreadNes;

	//
	CDebugInterface *GetDebugInterface(u8 emulatorType);
	int CountRunningDebugInterfaces();
	bool IsOnlyOneDebugInterfaceRunning();

	CColorsTheme *colorsTheme;
	
	CGuiButton *btnDone;
	bool ButtonClicked(CGuiButton *button);
	bool ButtonPressed(CGuiButton *button);
	
	CMainMenuBar *mainMenuBar;
	CGuiViewMessages *viewMessages;
	CViewMainMenu *viewC64MainMenu;
	CViewSettingsMenu *viewC64SettingsMenu;
	CViewC64KeyMap *viewC64KeyMap;
	CViewKeyboardShortcuts *viewKeyboardShortcuts;
	CViewSnapshots *viewC64Snapshots;
	CViewColodore *viewColodore;
	CViewAbout *viewAbout;
	CGuiViewUiDebug *viewUiDebug;


	int currentScreenLayoutId;
	
	CSlrFont *fontDisassembly;
	CSlrFont *fontDisassemblyInverted;
	
	//
	void InitViceViews();
	CViewC64Screen *viewC64Screen;
//	CViewC64ScreenWrapper *viewC64ScreenWrapper;
	CViewC64ScreenViewfinder *viewC64ScreenViewfinder;
	
	CViewMemoryMap *viewC64MemoryMap;
	CViewMemoryMap *viewDrive1541MemoryMap;
	
	CViewDataDump *viewC64MemoryDataDump;
	CViewDataWatch *viewC64MemoryDataWatch;
	CViewDataDump *viewDrive1541MemoryDataDump;
	CViewDataWatch *viewDrive1541MemoryDataWatch;

	CViewDataDump *viewC64MemoryDataDump2;
	CViewDataDump *viewC64MemoryDataDump3;

	CViewDataDump *viewDrive1541MemoryDataDump2;
	CViewDataDump *viewDrive1541MemoryDataDump3;

	CViewMemoryMap *viewC64CartridgeMemoryMap;
	CViewDataDump *viewC64CartridgeMemoryDataDump;
	
	CViewDisassembly *viewC64Disassembly;
	CViewDisassembly *viewC64Disassembly2;
	CViewDisassembly *viewDrive1541Disassembly;
	CViewDisassembly *viewDrive1541Disassembly2;
	
	CViewBreakpoints *viewC64BreakpointsPC;
	CViewBreakpoints *viewC64BreakpointsMemory;
	CViewBreakpoints *viewC64BreakpointsRaster;
	CViewBreakpoints *viewC64BreakpointsDrive1541PC;
	CViewBreakpoints *viewC64BreakpointsDrive1541Memory;

	CViewSourceCode *viewC64SourceCode;
	
	CViewC64StateCIA *viewC64StateCIA;
	CViewC64StateSID *viewC64StateSID;
	CViewC64StateVIC *viewC64StateVIC;
	CViewDrive1541StateVIA *viewDrive1541StateVIA;
	CViewC64StateREU *viewC64StateREU;
	CViewEmulationCounters *viewC64EmulationCounters;

	CViewEmulationState *viewEmulationState;

	CViewTimeline *viewC64Timeline;
	CViewC64VicDisplay *viewC64VicDisplay;
	CViewC64VicControl *viewC64VicControl;
	
	CViewC64AllGraphics *viewC64AllGraphics;
	
	CViewC64SidTrackerHistory *viewC64SidTrackerHistory;
	CViewC64SidPianoKeyboard *viewC64SidPianoKeyboard;
	
	CViewMonitorConsole *viewC64MonitorConsole;
	
	CViewC64StateCPU *viewC64StateCPU;
	CViewDrive1541StateCPU *viewDrive1541StateCPU;
	
	CViewDrive1541FileD64 *viewDrive1541FileD64;
	
	// VIC Editor
	CViewVicEditor *viewVicEditor;
	
	// Atari
	void InitAtari800Views();
	CViewAtariScreen *viewAtariScreen;
	CViewDisassembly *viewAtariDisassembly;
	CViewBreakpoints *viewAtariBreakpointsPC;
	CViewBreakpoints *viewAtariBreakpointsMemory;
//	CViewBreakpoints *viewAtariBreakpointsRaster;
	CViewSourceCode *viewAtariSourceCode;
	CViewDataDump *viewAtariMemoryDataDump;
	CViewDataWatch *viewAtariMemoryDataWatch;
	CViewMemoryMap *viewAtariMemoryMap;
	CViewAtariStateCPU *viewAtariStateCPU;
	CViewAtariStateANTIC *viewAtariStateANTIC;
	CViewAtariStatePIA *viewAtariStatePIA;
	CViewAtariStateGTIA *viewAtariStateGTIA;
	CViewAtariStatePOKEY *viewAtariStatePOKEY;
	CViewMonitorConsole *viewAtariMonitorConsole;
	CViewEmulationCounters *viewAtariEmulationCounters;
	CViewSnapshots *viewAtariSnapshots;
	CViewTimeline *viewAtariTimeline;

	// NES
	void InitNestopiaViews();
	CViewNesScreen *viewNesScreen;
	CViewNesStateCPU *viewNesStateCPU;
	CViewNesStateAPU *viewNesStateAPU;
	CViewNesPianoKeyboard *viewNesPianoKeyboard;
	CViewNesStatePPU *viewNesStatePPU;
	CViewNesPpuPatterns *viewNesPpuPatterns;
	CViewNesPpuNametables *viewNesPpuNametables;
	CViewNesPpuAttributes *viewNesPpuAttributes;
	CViewNesPpuOam *viewNesPpuOam;
	CViewNesPpuPalette *viewNesPpuPalette;
	CViewDisassembly *viewNesDisassembly;
	CViewBreakpoints *viewNesBreakpointsPC;
	CViewBreakpoints *viewNesBreakpointsMemory;
	CViewSourceCode *viewNesSourceCode;
	CViewMemoryMap *viewNesMemoryMap;
	CViewDataDump *viewNesMemoryDataDump;
	CViewDataWatch *viewNesMemoryDataWatch;
	CViewMemoryMap *viewNesPpuNametableMemoryMap;
	CViewDataDump *viewNesPpuNametableMemoryDataDump;
	CViewMonitorConsole *viewNesMonitorConsole;
	CViewEmulationCounters *viewNesEmulationCounters;
	CViewInputEvents *viewNesInputEvents;
	CViewSnapshots *viewNesSnapshots;
	CViewTimeline *viewNesTimeline;

	//
	CGuiViewProgressBarWindow *viewProgressBarWindow;
	
	//
	bool isDataDirectlyFromRAM;
	
	// TODO: these below are C64 related, move to debug interface
	// updated every render frame
	vicii_cycle_state_t currentViciiState;
	
	// state to show
	vicii_cycle_state_t viciiStateToShow;
	
	// current colors D020-D02E for displaying states
	u8 colorsToShow[0x0F];
	u8 colorToShowD800;
	
	int c64RasterPosToShowX;
	int c64RasterPosToShowY;
	int c64RasterPosCharToShowX;
	int c64RasterPosCharToShowY;
	
	void UpdateViciiColors();
	
	// JukeBox playlist
	CViewJukeboxPlaylist *viewJukeboxPlaylist;
	
	// move me and generalize me
	void InitViceC64();
	void InitAtari800();
	void InitNestopia();
	
	// TODO: generalize the below
	void StartEmulationThread(CDebugInterface *debugInterface);
	void StartViceC64EmulationThread();
	void StartAtari800EmulationThread();
	void StartNestopiaEmulationThread();

	void StopEmulationThread(CDebugInterface *debugInterface);
	void StopViceC64EmulationThread();
	void StopAtari800EmulationThread();
	void StopNestopiaEmulationThread();

	//
	
	void InitViews();
	
	//
	void InitJukebox(CSlrString *jukeboxJsonFilePath);
		
	int guiRenderFrameCounter;
//	int nextScreenUpdateFrame;
	
	//
	void EmulationStartFrameCallback(CDebugInterface *debugInterface);
	
	//
	void AddC64DebugCode();

	C64KeyboardShortcuts *keyboardShortcuts;
	bool ProcessKeyboardShortcut(u32 zone, u8 actionType, CSlrKeyboardShortcut *keyboardShortcut);
	
	void SwitchIsMulticolorDataDump();
	void SetIsMulticolorDataDump(bool isMultiColor);
	void SwitchIsShowRasterBeam();
	
	void StepOverInstruction();
	void StepOneCycle();
	void StepBackInstruction();
	void RunContinueEmulation();
	void HardReset();
	void SoftReset();
	
	void SwitchIsDataDirectlyFromRam();
	void SwitchIsDataDirectlyFromRam(bool isFromRam);

	//
	CViewDisassembly *GetActiveDisassembleView();
	
	
	std::vector<CGuiView *> traversalOfViews;
	bool CanSelectView(CGuiView *view);
	void MoveFocusToNextView();
	void MoveFocusToPrevView();
	
	
	// fonts
	CSlrFont *fontCBM1;
	CSlrFont *fontCBM2;
	CSlrFont *fontCBMShifted;
	
	CSlrFont *fontAtari;
	
	ImFont *imFontDefault;
//	ImFont *imFontPro;
//	ImFont *imFontSweet16;

	const char *defaultFontPath;
	float defaultFontSize;
	int defaultFontOversampling;

	void CreateFonts();
	void Create8BitFonts();
	void CreateDefaultUIFont();
	void UpdateDefaultUIFontFromSettings();
	
	void SetViewportsEnable(bool viewportsEnable);

	volatile bool isShowingRasterCross;
	
	virtual void ApplicationEnteredBackground();
	virtual void ApplicationEnteredForeground();
	virtual void ApplicationShutdown();

	
	// TODO: move this below to proper debug interfaces:
	void MapC64MemoryToFile(char *filePath);
	void UnMapC64MemoryFromFile();
	uint8 *mappedC64Memory;
	void *mappedC64MemoryDescriptor;

	//
//	void MapAtariMemoryToFile(char *filePath);
//	void UnMapAtariMemoryFromFile();
//	uint8 *mappedAtariMemory;
//	void *mappedAtariMemoryDescriptor;

	
	//
	void DefaultSymbolsStore();
	void SerializeAllEmulatorsSymbols(Hjson::Value hjsonRoot, bool storeLabels, bool storeWatches, bool storeBreakpoints);
	void DefaultSymbolsRestore();
	void DeserializeAllEmulatorsSymbols(Hjson::Value hjsonRoot, bool restoreLabels, bool restoreWatches, bool restoreBreakpoints);
	bool firstStoreDefaultSymbols;
	
	//
	std::list<u32> keyDownCodes;
	
	// mouse cursor for scrolling where cursor is
	float mouseCursorX, mouseCursorY;
		
	//
	virtual void SharedMemorySignalCallback(CByteBuffer *sharedMemoryData);

	void InitRasterColors();
	
	void CheckMouseCursorVisibility();
	void ShowMouseCursor();

	void GoFullScreen(CGuiView *view);
	void ToggleFullScreen(CGuiView *view);
	
	//
	bool IsMouseCursorOnTimeline();
	int mouseCursorVisibilityCounter;
	int mouseCursorNumFramesToHideCursor;
	
	//
	void ShowMainScreen();
	
	// open/save dialogs
	void ShowDialogOpenFile(CSystemFileDialogCallback *callback, std::list<CSlrString *> *extensions,
							CSlrString *defaultFolder,
							CSlrString *windowTitle);

	void ShowDialogSaveFile(CSystemFileDialogCallback *callback, std::list<CSlrString *> *extensions,
							CSlrString *defaultFileName, CSlrString *defaultFolder,
							CSlrString *windowTitle);
	
	CGuiViewSaveFile *viewSaveFile;
	CGuiViewSelectFile *viewSelectFile;
	CGuiView *fileDialogPreviousView;
	CSystemFileDialogCallback *systemFileDialogCallback;
	
	virtual void FileSelected(CSlrString *filePath);
	virtual void FileSelectionCancelled();
	
	virtual void SaveFileSelected(CSlrString *fullFilePath, CSlrString *fileName);
	virtual void SaveFileSelectionCancelled();
	
	bool isSoundMuted;
	void ToggleSoundMute();
	void SetSoundMute(bool isMuted);
	void UpdateSIDMute();
		
	//
	void CreateEmulatorPlugins();
	void RegisterEmulatorPlugin(CDebuggerEmulatorPlugin *emuPlugin);
	
	// async show message
	CSlrMutex *mutexShowMessage;
	void ShowMessage(CSlrString *showMessage);
	void ShowMessage(const char *fmt, ...);

	//
	virtual void GlobalDropFileCallback(char *filePath, bool consumedByView);
	
	//
	CRecentlyOpenedFiles *recentlyOpenedFiles;
	virtual void RecentlyOpenedFilesCallbackSelectedMenuItem(CSlrString *filePath);
		
	//
	
	char *ATRD_GetPathForRoms_IMPL();

};

extern CViewC64 *viewC64;

// drag & drop callbacks
extern unsigned long c64dStartupTime;

class CUiThreadTaskSetDefaultUiFont : public CUiThreadTaskCallback
{
public:
	virtual void RunUIThreadTask();
};

class CUiThreadTaskSetViewportsEnable : public CUiThreadTaskCallback
{
public:
	bool viewportsEnable;
	virtual void RunUIThreadTask();
};

#endif //_GUI_C64DEMO_
