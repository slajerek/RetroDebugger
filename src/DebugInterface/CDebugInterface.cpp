#include "CDebugInterface.h"
#include "CDebugInterfaceTask.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CViewC64.h"
#include "SYS_Threading.h"
#include "C64SettingsStorage.h"
#include "CSnapshotsManager.h"
#include "CDebuggerEmulatorPlugin.h"
#include "CViewDisassembly.h"

CDebugInterface::CDebugInterface(CViewC64 *viewC64)
{
	this->viewC64 = viewC64;

	snapshotsManager = NULL;
	symbols = NULL;
	codeMonitorCallback = NULL;

	isDebugOn = true;
	
	isRunning = false;
	isSelected = false;
	
	breakpointsMutex = new CSlrMutex("CDebugInterface::breakpointsMutex");
	renderScreenMutex = new CSlrMutex("CDebugInterface::renderScreenMutex");
	ioMutex = new CSlrMutex("CDebugInterface::ioMutex");
	tasksMutex = new CSlrMutex("CDebugInterface::tasksMutex");

	screenSupersampleFactor = c64SettingsScreenSupersampleFactor;
	
	emulationFrameCounter = 0;
	
	viewScreen = NULL;
	
	this->debugMode = DEBUGGER_MODE_RUNNING;
}

CDebugInterface::~CDebugInterface()
{
}

void CDebugInterface::Shutdown()
{
	this->SetDebugMode(DEBUGGER_MODE_SHUTDOWN);
}

int CDebugInterface::GetEmulatorType()
{
	return EMULATOR_TYPE_UNKNOWN;
}

CSlrString *CDebugInterface::GetEmulatorVersionString()
{
	return NULL;
}

const char *CDebugInterface::GetPlatformNameString()
{
	return NULL;
}

float CDebugInterface::GetEmulationFPS()
{
	return -1;
}

void CDebugInterface::RunEmulationThread()
{
}

void CDebugInterface::RestartAudio()
{
}

void CDebugInterface::RefreshSync()
{
}

void CDebugInterface::InitPlugins()
{
	for (std::list<CDebuggerEmulatorPlugin *>::iterator it = this->plugins.begin(); it != this->plugins.end(); it++)
	{
		CDebuggerEmulatorPlugin *plugin = *it;
		plugin->Init();
	}
}

// all cycles in frame finished, vsync
void CDebugInterface::DoVSync()
{
	emulationFrameCounter++;
	
//	LOGD("DoVSync: frame=%d", emulationFrameCounter);
	viewC64->EmulationStartFrameCallback(this);
}

// frame is painted on canvas and ready to be consumed
void CDebugInterface::DoFrame()
{
	for (std::list<CDebuggerEmulatorPlugin *>::iterator it = this->plugins.begin(); it != this->plugins.end(); it++)
	{
		CDebuggerEmulatorPlugin *plugin = *it;
		plugin->DoFrame();
	}
}

u64 CDebugInterface::GetMainCpuCycleCounter()
{
	LOGError("CDebugInterface::GetMainCpuCycleCounter: not implemented");
	return 0;
}

u64 CDebugInterface::GetCurrentCpuInstructionCycleCounter()
{
	return GetMainCpuCycleCounter();
}

void CDebugInterface::ResetMainCpuDebugCycleCounter()
{
	LOGError("CDebugInterface::ResetMainCpuDebugCycleCounter: not implemented");
}

u64 CDebugInterface::GetMainCpuDebugCycleCounter()
{
	LOGError("CDebugInterface::GetMainCpuDebugCycleCounter: not implemented");
	return 0;
}

u64 CDebugInterface::GetPreviousCpuInstructionCycleCounter()
{
	LOGError("CDebugInterface::GetPreviousCpuInstructionCycleCounter: not implemented");
	return 0;
}

void CDebugInterface::ResetEmulationFrameCounter()
{
	this->emulationFrameCounter = 0;
}


unsigned int CDebugInterface::GetEmulationFrameNumber()
{
	return this->emulationFrameCounter;
}

void CDebugInterface::CreateScreenData()
{
	screenImage = new CImageData(512 * this->screenSupersampleFactor, 512 * this->screenSupersampleFactor, IMG_TYPE_RGBA);
	screenImage->AllocImage(false, true);
}

void CDebugInterface::RefreshScreenNoCallback()
{
	SYS_FatalExit("CDebugInterface::RefreshScreenNoCallback");
}


void CDebugInterface::SetSupersampleFactor(int factor)
{
	LOGM("CDebugInterface::SetSupersampleFactor: %d", factor);
	this->LockRenderScreenMutex();
	
	this->screenSupersampleFactor = factor;
	
	delete screenImage;
	CreateScreenData();
	
	this->UnlockRenderScreenMutex();
}

CImageData *CDebugInterface::GetScreenImageData()
{
//	LOGD("CDebugInterface::GetScreenImageData");
	return this->screenImage;
}

CDebugDataAdapter *CDebugInterface::GetDataAdapter()
{
	SYS_FatalExit("CDebugInterface::GetDataAdapter");
	return NULL;
}

bool CDebugInterface::LoadExecutable(char *fullFilePath)
{
	SYS_FatalExit("CDebugInterface::LoadExecutable");
	return false;
}

bool CDebugInterface::MountDisk(char *fullFilePath, int diskNo, bool readOnly)
{
	SYS_FatalExit("CDebugInterface::MountDisk");
	return false;
}

bool CDebugInterface::LoadFullSnapshot(char *filePath)
{
	return false;
}

void CDebugInterface::SaveFullSnapshot(char *filePath)
{
}

bool CDebugInterface::LoadChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
	return false;
}

bool CDebugInterface::SaveChipsSnapshotSynced(CByteBuffer *byteBuffer)
{
	return false;
}

bool CDebugInterface::LoadDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	return false;
}

bool CDebugInterface::SaveDiskDataSnapshotSynced(CByteBuffer *byteBuffer)
{
	return false;
}

bool CDebugInterface::IsDriveDirtyForSnapshot()
{
	SYS_FatalExit("CDebugInterface::IsDriveDirtyForSnapshot");
	return false;
}

void CDebugInterface::ClearDriveDirtyForSnapshotFlag()
{
	SYS_FatalExit("CDebugInterface::ClearDriveDirtyForSnapshotFlag");
}

int CDebugInterface::GetScreenSizeX()
{
	return -1;
}

int CDebugInterface::GetScreenSizeY()
{
	return -1;
}

// keyboard & joystick mapper
bool CDebugInterface::KeyboardDown(uint32 mtKeyCode)
{
	return false;
}

bool CDebugInterface::KeyboardUp(uint32 mtKeyCode)
{
	return false;
}

void CDebugInterface::JoystickDown(int port, uint32 axis)
{
}

void CDebugInterface::JoystickUp(int port, uint32 axis)
{
}

void CDebugInterface::MouseDown(float x, float y)
{
}

void CDebugInterface::MouseMove(float x, float y)
{
}

void CDebugInterface::MouseUp(float x, float y)
{
}

// input events for snapshot manager
CByteBuffer *CDebugInterface::GetInputEventsBufferForCurrentCycle()
{
	return this->snapshotsManager->StoreNewInputEventsSnapshotAtCurrentCycle();
}

// this is called by CSnapshotsManager when input events at current cycle exist and need to be replayed
void CDebugInterface::ReplayInputEventsFromSnapshotsManager(CByteBuffer *inputEventsBuffer)
{
	LOGTODO("CDebugInterface::ReplayInputEventsFromSnapshotsManager");
}

// this is called by CSnapshotManager to replay events at current cycle
void ReplayInputEventsFromSnapshotsManager(CByteBuffer *byteBuffer);

// state
int CDebugInterface::GetCpuPC()
{
	SYS_FatalExit("CDebugInterface::GetCpuPC");
	return -1;
}

void CDebugInterface::GetWholeMemoryMap(uint8 *buffer)
{
	SYS_FatalExit("CDebugInterface::GetWholeMemoryMap");
}

void CDebugInterface::GetWholeMemoryMapFromRam(uint8 *buffer)
{
	SYS_FatalExit("CDebugInterface::GetWholeMemoryMap");
}

//
void CDebugInterface::SetDebugMode(uint8 debugMode)
{
	this->debugMode = debugMode;
}

uint8 CDebugInterface::SetDebugModeBlockedWait(uint8 debugMode)
{
	u8 currentDebugMode = GetDebugMode();
	
	LockMutex();
	this->SetDebugMode(debugMode);
	UnlockMutex();
	
	// TODO: CDebugInterface::SetDebugModeBlockedWait add debug states, running cpu, paused, rewinding, etc.
	SYS_Sleep(500);
	
	return currentDebugMode;
}

uint8 CDebugInterface::GetDebugMode()
{
	return this->debugMode;
}

void CDebugInterface::Reset()
{
	SYS_FatalExit("CDebugInterface::Reset");
}

void CDebugInterface::HardReset()
{
	SYS_FatalExit("CDebugInterface::HardReset");
}

void CDebugInterface::SetDebugOn(bool debugOn)
{
	this->isDebugOn = debugOn;
}

void CDebugInterface::SupportsBreakpoints(bool *writeBreakpoint, bool *readBreakpoint)
{
	*writeBreakpoint = false;
	*readBreakpoint = false;
}

bool CDebugInterface::GetSettingIsWarpSpeed()
{
	return false;
}

void CDebugInterface::SetSettingIsWarpSpeed(bool isWarpSpeed)
{
	LOGError("CDebugInterface::SetSettingIsWarpSpeed: emulator %s not implemented", GetPlatformNameString());
}

//
// make jmp without resetting CPU depending on dataAdapter
void CDebugInterface::MakeJmpNoReset(CDataAdapter *dataAdapter, uint16 addr)
{
	SYS_FatalExit("CDebugInterface::MakeJmpNoReset");
}

// make jmp and reset CPU
void CDebugInterface::MakeJmpAndReset(uint16 addr)
{
	SYS_FatalExit("CDebugInterface::MakeJmpAndReset");
}

void CDebugInterface::ClearTemporaryBreakpoint()
{
	if (this->GetDebugMode() == DEBUGGER_MODE_RUNNING)
	{
		symbols->ClearTemporaryBreakpoint();
	}
}

void CDebugInterface::StepOverInstruction()
{
	this->ClearTemporaryBreakpoint();
	this->snapshotsManager->CancelRestore();
	this->SetDebugMode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
}

void CDebugInterface::StepOneCycle()
{
	this->ClearTemporaryBreakpoint();
	this->snapshotsManager->CancelRestore();
	this->SetDebugMode(DEBUGGER_MODE_RUN_ONE_CYCLE);
}

void CDebugInterface::RunContinueEmulation()
{
	this->ClearTemporaryBreakpoint();
	this->snapshotsManager->CancelRestore();
	this->SetDebugMode(DEBUGGER_MODE_RUNNING);
}

// view breakpoints
void CDebugInterface::UpdateRenderBreakpoints()
{
	if (symbols)
	{
		symbols->UpdateRenderBreakpoints();
	}
}

//
void CDebugInterface::AddVSyncTask(CDebugInterfaceTask *task)
{
	this->LockTasksMutex();
	vsyncTasks.push_back(task);
	this->UnlockTasksMutex();
}

// tasks to be executed when emulation is safe in vsync, i.e. completed frame rendering
void CDebugInterface::ExecuteVSyncTasks()
{
	this->LockTasksMutex();
	while(!vsyncTasks.empty())
	{
		CDebugInterfaceTask *task = vsyncTasks.front();
		vsyncTasks.pop_front();
		task->ExecuteTask();
		delete task;
	}
	this->UnlockTasksMutex();
}

// tasks to be executed when emulation is safe in debugger interrupt (depends on emulation, but f.e. cpu is about to execute instruction)
void CDebugInterface::AddCpuDebugInterruptTask(CDebugInterfaceTask *task)
{
	this->LockTasksMutex();
	cpuDebugInterruptTasks.push_back(task);
	this->UnlockTasksMutex();
}

// this is run every cycle to check if there are tasks to be executed from UI
void CDebugInterface::ExecuteDebugInterruptTasks()
{
//	LOGD("%s ExecuteDebugInterruptTasks cycle=%d", GetPlatformNameString(), GetMainCpuCycleCounter());
	
	snapshotsManager->CheckInputEventsAtCurrentCycle();

	this->LockTasksMutex();
	while(!cpuDebugInterruptTasks.empty())
	{
		CDebugInterfaceTask *task = cpuDebugInterruptTasks.front();
		cpuDebugInterruptTasks.pop_front();
		task->ExecuteTask();
		delete task;
	}
	this->UnlockTasksMutex();
}

//
void CDebugInterface::RegisterPlugin(CDebuggerEmulatorPlugin *plugin)
{
	this->plugins.push_back(plugin);
}

void CDebugInterface::RemovePlugin(CDebuggerEmulatorPlugin *plugin)
{
	this->plugins.remove(plugin);
}

//
bool CDebugInterface::IsCodeMonitorSupported()
{
	return false;
}

void CDebugInterface::SetCodeMonitorCallback(CDebugInterfaceCodeMonitorCallback *callback)
{
	this->codeMonitorCallback = callback;	
}

CSlrString *CDebugInterface::GetCodeMonitorPrompt()
{
	// monitor is not supported
	return NULL;
}

bool CDebugInterface::ExecuteCodeMonitorCommand(CSlrString *commandStr)
{
	// monitor is not supported
	return false;
}

bool CDebugInterface::IsEmulationRunning()
{
	return isRunning;
}

void CDebugInterface::AddView(CGuiView *view)
{
	views.push_back(view);
}

void CDebugInterface::AddMenuItem(CDebugInterfaceMenuItem *menuItem)
{
	menuItems.push_back(menuItem);
}

//
CGuiView *CDebugInterface::GetViewScreen()
{
	return viewScreen;
}

CViewMemoryMap *CDebugInterface::GetViewMemoryMap()
{
	CDebugDataAdapter *dataAdapter = GetDataAdapter();
	if (dataAdapter == NULL)
	{
		LOGError("CDebugInterface::GetViewMemoryMap: dataAdapter is NULL");
		return NULL;
	}
	return dataAdapter->viewMemoryMap;
}

CViewTimeline *CDebugInterface::GetViewTimeline()
{
	if (this->snapshotsManager == NULL)
	{
		LOGError("CDebugInterface::GetViewTimeline: snapshotsManager is NULL");
		return NULL;
	}
	return this->snapshotsManager->viewTimeline;
}

// TODO: ADD "#define DEBUGMUTEX" and push/pull names of locks here, list to be displayed when this locks here again
void CDebugInterface::LockMutex()
{
	//	LOGD("CDebugInterface::LockMutex");
	breakpointsMutex->Lock();
}

void CDebugInterface::UnlockMutex()
{
	//	LOGD("CDebugInterface::UnlockMutex");
	breakpointsMutex->Unlock();
}

void CDebugInterface::LockRenderScreenMutex()
{
	renderScreenMutex->Lock();
}

void CDebugInterface::UnlockRenderScreenMutex()
{
	renderScreenMutex->Unlock();
}

void CDebugInterface::LockIoMutex()
{
	ioMutex->Lock();
}

void CDebugInterface::UnlockIoMutex()
{
	ioMutex->Unlock();
}

void CDebugInterface::LockTasksMutex()
{
	tasksMutex->Lock();
}

void CDebugInterface::UnlockTasksMutex()
{
	tasksMutex->Unlock();
}

void CDebugInterfaceCodeMonitorCallback::CodeMonitorCallbackPrintLine(CSlrString *printLine)
{
	LOGError("CDebugInterfaceCodeMonitorCallback::CodeMonitorCallbackPrintLine: not implemented callback");
}

void CDebugInterface::ShowMessageBox(const char *title, const char *message)
{
	guiMain->ShowMessageBox(title, message);
}

