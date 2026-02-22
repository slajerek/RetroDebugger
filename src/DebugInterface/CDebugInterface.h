#ifndef _CDEBUGINTERFACE_H_
#define _CDEBUGINTERFACE_H_

#include "CDebugBreakpoint.h"
#include "CDebugBreakpointAddr.h"
#include "CDebugBreakpointData.h"
#include "CDebugBreakpointRasterLine.h"
#include "CDebugBreakpointsAddr.h"
#include "CDebugBreakpointsData.h"
#include "CDebugDataAdapter.h"
#include "CByteBuffer.h"
#include "DebuggerDefs.h"
#include "EmulatorsConfig.h"
#include "CStackAnnotation.h"
#include "json.hpp"

#include <map>
#include <list>

class CGuiView;

class CViewC64;
class CSlrMutex;
class CImageData;

class CDebuggerEmulatorPlugin;
class CSnapshotsManager;
class CDebugSymbols;
class CDebugSymbolsSegment;

class CViewEmulatorScreen;
class CViewDisassembly;
class CViewBreakpoints;
class CViewDataWatch;

class CDebugInterfaceMenuItem;

class CDebugInterfaceTask;
class CViewDataMap;
class CViewTimeline;
class CDebugEventsHistory;

class CDebuggerApi;
class CDebuggerServerApi;

class CDebugInterfaceCodeMonitorCallback
{
public:
	virtual void CodeMonitorCallbackPrintLine(CSlrString *printLine);
};

// abstract class
class CDebugInterface
{
public:
	CDebugInterface(CViewC64 *viewC64);
	virtual ~CDebugInterface();
	
	CViewC64 *viewC64;
	
	CSnapshotsManager *snapshotsManager;
	CDebugSymbols *symbols;

	virtual int GetEmulatorType();
	virtual CSlrString *GetEmulatorVersionString();
	virtual const char *GetPlatformNameString();
	virtual const char *GetPlatformNameEndpointString();
	
	virtual float GetEmulationFPS();
	
	bool isRunning;
	virtual bool IsEmulationRunning();
	
	bool isSelected;
	
	unsigned int emulationFrameCounter;

	virtual void RunEmulationThread();
	virtual void RestartAudio();
	// reset a/v sync
	virtual void RefreshSync();

	virtual void InitPlugins();
	
	// all cycles in frame finished, vsync
	virtual void DoVSync();

	// frame is painted on canvas and ready to be consumed
	virtual void DoFrame();
	
	//
	virtual void ClearDebugMarkers();
	virtual void ClearHistory();

	// run various tasks when emulation is in defined state
	
	// tasks to be executed when emulation just finished rendering frame
	std::list<CDebugInterfaceTask *> vsyncTasks;
	virtual void AddVSyncTask(CDebugInterfaceTask *task);
	virtual void ExecuteVSyncTasks();
	
	// tasks to be executed when emulation is about to execute instruction
	std::list<CDebugInterfaceTask *> cpuDebugInterruptTasks;
	virtual void AddCpuDebugInterruptTask(CDebugInterfaceTask *task);
	virtual void ExecuteDebugInterruptTasks();

	// this is main emulation cpu cycle counter
	virtual u64 GetMainCpuCycleCounter();
	virtual u64 GetCurrentCpuInstructionCycleCounter();
	virtual u64 GetPreviousCpuInstructionCycleCounter();
	
	// resettable counters for debug purposes
	virtual void ResetMainCpuDebugCycleCounter();
	virtual u64 GetMainCpuDebugCycleCounter();
	virtual void ResetEmulationFrameCounter();
	virtual unsigned int GetEmulationFrameNumber();
	
	virtual void RefreshScreenNoCallback();

	virtual int GetScreenSizeX();
	virtual int GetScreenSizeY();
	
	CImageData *screenImageData;
	virtual void CreateScreenData();
	int screenSupersampleFactor;
	virtual void SetSupersampleFactor(int factor);
	virtual CImageData *GetScreenImageData();
	
	// keyboard & joystick mapper
	virtual bool KeyboardDown(uint32 mtKeyCode);
	virtual bool KeyboardUp(uint32 mtKeyCode);
	
	virtual void JoystickDown(int port, uint32 axis);
	virtual void JoystickUp(int port, uint32 axis);
	
	virtual void MouseDown(float x, float y);
	virtual void MouseMove(float x, float y);
	virtual void MouseUp(float x, float y);
	
	// force key up modifier keys when processing keyboard shortcut
	virtual void KeyUpModifierKeys(bool isShift, bool isAlt, bool isControl);

	// state
	virtual int GetCpuPC();
	
	virtual void GetWholeMemoryMap(uint8 *buffer);
	virtual void GetWholeMemoryMapFromRam(uint8 *buffer);
	
	//
	virtual void SetDebugMode(uint8 debugMode);
	virtual uint8 SetDebugModeBlockedWait(uint8 debugMode);
	virtual uint8 GetDebugMode();
	virtual void PauseEmulationBlockedWait();

	//
	virtual void ResetSoft();
	virtual void ResetHard();
	
	virtual void DetachEverything();

	//
	virtual bool LoadExecutable(char *fullFilePath);
	virtual bool MountDisk(char *fullFilePath, int diskNo, bool readOnly);

	//
	virtual bool LoadFullSnapshot(char *filePath);
	virtual void SaveFullSnapshot(char *filePath);

	// these calls should be synced with CPU IRQ so snapshot store or restore is allowed
	// store CHIPS only snapshot, not including DISK DATA
	virtual bool LoadChipsSnapshotSynced(CByteBuffer *byteBuffer);
	virtual bool SaveChipsSnapshotSynced(CByteBuffer *byteBuffer);
	// store DISK DATA only snapshot, without CHIPS
	virtual bool LoadDiskDataSnapshotSynced(CByteBuffer *byteBuffer);
	virtual bool SaveDiskDataSnapshotSynced(CByteBuffer *byteBuffer);
	
	// controls if we should also store disk drive snapshot when snapshot interval is hit
	// this is to allow only periodic disk drive snapshot storing, only when disk was changed or new data on disk was saved
	// when we restore snapshot, we will restore disk contents first
	virtual bool IsDriveDirtyForSnapshot();
	virtual void ClearDriveDirtyForSnapshotFlag();

	// events for snapshot manager
	// returns CByteBuffer that can be filled with events to be stored at current cycle
	virtual CByteBuffer *GetInputEventsBufferForCurrentCycle();
	
	// this is called by CSnapshotManager to replay events at current cycle
	virtual void ReplayInputEventsFromSnapshotsManager(CByteBuffer *inputEventsBuffer);

	//
	virtual bool GetSettingIsWarpSpeed();
	virtual void SetSettingIsWarpSpeed(bool isWarpSpeed);

	// cpu control

	// make jmp without resetting CPU depending on dataAdapter
	virtual void MakeJmpNoReset(CDataAdapter *dataAdapter, uint16 addr);
	
	// make jmp and reset CPU
	virtual void MakeJmpAndReset(uint16 addr);
	
	virtual void ClearTemporaryBreakpoint();
	
	virtual void StepOverInstruction();
	virtual void StepOneCycle();
	virtual void StepOverSubroutine();
	
	virtual void RunContinueEmulation();

	// breakpoints
	virtual void SupportsBreakpoints(bool *writeBreakpoint, bool *readBreakpoint);
	bool isDebugOn;
	virtual void SetDebugOn(bool debugOn);

	virtual CDebugDataAdapter *GetDataAdapter();
	
	// note this may fallback to default data adapter if not implemented
	virtual CDebugDataAdapter *GetDataAdapterDirectRam();

	// view breakpoints
	virtual void UpdateRenderBreakpoints();

	// note: shall we move logic to create views to debug interface?
	std::list<CGuiView *> views;
	void AddView(CGuiView *view);
	std::list<CDebugInterfaceMenuItem *> menuItems;
	void AddMenuItem(CDebugInterfaceMenuItem *menuItem);
	
	// this is to get prompt and issue commands to native code monitor of the emulator (i.e. Vice's original monitor)
	virtual bool IsCodeMonitorSupported();
	
	CDebugInterfaceCodeMonitorCallback *codeMonitorCallback;
	virtual void SetCodeMonitorCallback(CDebugInterfaceCodeMonitorCallback *callback);
	
	// @returns NULL when monitor is not supported
	virtual CSlrString *GetCodeMonitorPrompt();
	virtual bool ExecuteCodeMonitorCommand(CSlrString *commandStr);

	virtual void Shutdown();

	//
	void ShowNotificationInfo(const char *message);
	void ShowNotificationError(const char *message);
	void ShowMessageBox(const char *title, const char *message);
	
	//
	std::list<CDebuggerEmulatorPlugin *> plugins;
	void RegisterPlugin(CDebuggerEmulatorPlugin *plugin);
	void RemovePlugin(CDebuggerEmulatorPlugin *plugin);
	
	// views
	CViewEmulatorScreen *viewScreen;
	virtual CViewEmulatorScreen *GetViewScreen();
	std::vector<CViewDataMap *> viewsMemoryMap;
	virtual void AddViewMemoryMap(CViewDataMap *viewMemoryMap);
	virtual CViewTimeline *GetViewTimeline();
	CViewDisassembly *viewDisassembly;
	virtual CViewDisassembly *GetViewDisassembly();
	
	// plugin api
	virtual CDebuggerApi *GetDebuggerApi();
	
	// debugger server api
	CDebuggerServerApi *debuggerServerApi;
	virtual CDebuggerServerApi *GetDebuggerServerApi();
	
	//
	CSlrMutex *breakpointsMutex;
	virtual void LockMutex();
	virtual void UnlockMutex();
	
	CSlrMutex *renderScreenMutex;
	virtual void LockRenderScreenMutex();
	virtual void UnlockRenderScreenMutex();
	
	CSlrMutex *ioMutex;
	virtual void LockIoMutex();
	virtual void UnlockIoMutex();
	
	CSlrMutex *tasksMutex;
	virtual void LockTasksMutex();
	virtual void UnlockTasksMutex();

	// stack annotations (per-byte tracking of what was pushed onto $0100-$01FF)
	CStackAnnotationData mainCpuStack;
	virtual const char *GetIrqSourceName(u8 source);

	//
protected:
	volatile int debugMode;

};


#endif
