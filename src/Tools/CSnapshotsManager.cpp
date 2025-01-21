#include "CSnapshotsManager.h"
#include "SYS_Threading.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CViewC64.h"
#include "CDebugMemory.h"
#include "SND_SoundEngine.h"
#include "C64SettingsStorage.h"
#include "CSlrFileFromOS.h"
#include "CSlrFileZlib.h"

#include "M_Circlebuf.h"

// TODO: rewriting map frame snapshot...  (?)
// DONE?: attach regular snapshots -> clear rewind data
// DONE?: store roms on cart change
// TODO: what to do with REU? 16MB can't be stored easily. add another "disk" snapshot? maybe incremental updates?
// TODO: refactor "disk" to "storage", in fact this is not disk only but cart rom also

// TODO: CPU JAM - un-jam when rewind ?
// DONE?: DUMP all snapshots & history => fix problem with cycle on frame# change

// this is to debug snapshots manager, as it is quite heavy it is normally switched off
#undef LOGS
//#define LOGS LOGD
#define LOGS {}
#pragma clang diagnostic ignored "-Wunused-value"
#pragma clang diagnostic ignored "-Wconversion"

int CSnapshotsManager::progressNumSnapshots = 0;
int CSnapshotsManager::progressCurrentSnapshot = 0;
float CSnapshotsManager::progressStoreOrRestore = 0.0f;	// for progress bar

CStoredSnapshot::CStoredSnapshot(CSnapshotsManager *manager, u8 snapshotType)
{
	this->manager = manager;
	byteBuffer = new CByteBuffer();

	this->frame = -1;
	this->cycle = -1;
	
	this->snapshotType = snapshotType;
}

CStoredSnapshot::~CStoredSnapshot()
{
}

void CStoredSnapshot::Use(u32 frame, u64 cycle)
{
	this->frame = frame;
	this->cycle = cycle;
}

void CStoredSnapshot::Clear()
{
	if (this->cycle != -1)
	{
//		LOGD("CStoredSnapshot::Clear: was cycle %d", this->cycle);
	}
	this->frame = -1;
	this->cycle = -1;
	byteBuffer->Clear();
}

void CStoredSnapshot::StoreToFile(CSlrFile *file)
{
	file->WriteU8(snapshotType);
	file->WriteU32(frame);
	file->WriteU64(cycle);
	file->WriteByteBuffer(byteBuffer);
}

void CStoredSnapshot::RestoreFromFile(CSlrFile *file)
{
//	snapshotType = file->ReadU8();	this is read by manager
	frame = file->ReadU32();
	cycle = file->ReadU64();
	file->ReadByteBuffer(byteBuffer);
}

CStoredDiskSnapshot::CStoredDiskSnapshot(CSnapshotsManager *manager, u32 frame, u64 cycle)
: CStoredSnapshot(manager, SNAPSHOT_TYPE_DISK)
{
	numLinkedChipsSnapshots = 0;
	Use(frame, cycle);
}

CStoredDiskSnapshot::CStoredDiskSnapshot(CSnapshotsManager *manager, CSlrFile *file)
: CStoredSnapshot(manager, SNAPSHOT_TYPE_DISK)
{
	numLinkedChipsSnapshots = 0;
	RestoreFromFile(file);
}


void CStoredDiskSnapshot::AddReference()
{
	numLinkedChipsSnapshots++;
}

void CStoredDiskSnapshot::RemoveReference()
{
//	LOGD("CStoredDiskSnapshot::RemoveReference: cycle=%d numLinkedChipsSnapshots=%d", cycle, numLinkedChipsSnapshots);
	numLinkedChipsSnapshots--;

//	manager->DebugPrintDiskSnapshots();
	
	if (numLinkedChipsSnapshots == 0)
	{
//		LOGD("CStoredDiskSnapshot::RemoveReference linked=0, Clear");
		manager->diskSnapshotsByFrame.erase(this->frame);
		manager->diskSnapshotsByCycle.erase(this->frame);
		manager->diskSnapshotsToReuse.push_back(this);
		Clear();
	}

//	LOGD("CStoredDiskSnapshot::RemoveReference finished: numLinkedChipsSnapshots=%d", numLinkedChipsSnapshots);
}

void CStoredDiskSnapshot::StoreToFile(CSlrFile *file)
{
	LOGD("CStoredDiskSnapshot::StoreToFile: cycle=%d", cycle);
	if (cycle == -1)
	{
		LOGError("CStoredDiskSnapshot::StoreToFile: cycle=-1");
	}
	CStoredSnapshot::StoreToFile(file);
//	file->WriteUnsignedInt(numLinkedChipsSnapshots);
}

void CStoredDiskSnapshot::RestoreFromFile(CSlrFile *file)
{
	LOGD("CStoredDiskSnapshot::RestoreFromFile: cycle=%d", cycle);
	CStoredSnapshot::RestoreFromFile(file);
//	numLinkedChipsSnapshots = file->ReadUnsignedInt();
}

CStoredChipsSnapshot::CStoredChipsSnapshot(CSnapshotsManager *manager, u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot)
: CStoredSnapshot(manager, SNAPSHOT_TYPE_CHIPS)
{
	Use(frame, cycle, diskSnapshot);
}

CStoredChipsSnapshot::CStoredChipsSnapshot(CSnapshotsManager *manager, CSlrFile *file)
: CStoredSnapshot(manager, SNAPSHOT_TYPE_CHIPS)
{
	this->diskSnapshot = NULL;
	RestoreFromFile(file);
}

void CStoredChipsSnapshot::Use(u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot)
{
	this->frame = frame;
	this->cycle = cycle;
	this->diskSnapshot = diskSnapshot;
	this->diskSnapshot->AddReference();
}


void CStoredChipsSnapshot::Clear()
{
	if (this->diskSnapshot)
	{
		this->diskSnapshot->RemoveReference();
		this->diskSnapshot = NULL;
	}
	
	CStoredSnapshot::Clear();
}

void CStoredChipsSnapshot::StoreToFile(CSlrFile *file)
{
	CStoredSnapshot::StoreToFile(file);

	LOGD("CStoredChipsSnapshot::StoreToFile: cycle=%d diskSnapshot=%x", cycle, diskSnapshot);
	if (this->diskSnapshot)
	{
		LOGD("... diskSnapshot cycle=%d", this->diskSnapshot->cycle);
		file->WriteBool(true);
		file->WriteU64(this->diskSnapshot->cycle);
	}
	else
	{
		file->WriteBool(false);
	}
}

void CStoredChipsSnapshot::RestoreFromFile(CSlrFile *file)
{
	CStoredSnapshot::RestoreFromFile(file);
	
	LOGD("CStoredChipsSnapshot::RestoreFromFile cycle=%d", cycle);
	bool hasDiskSnapshot = file->ReadBool();
	LOGD("... hasDiskSnapshot=%s", STRBOOL(hasDiskSnapshot));
	if (hasDiskSnapshot)
	{
		u64 diskSnapshotCycle = file->ReadU64();
		LOGD("... diskSnapshotCycle=%d", diskSnapshotCycle);
		
		std::map<u64, CStoredDiskSnapshot *>::iterator it =  manager->diskSnapshotsByCycle.find(diskSnapshotCycle);
		if (it == manager->diskSnapshotsByCycle.end())
		{
			LOGError("CStoredChipsSnapshot::RestoreFromFile: disk snapshot not found cycle=%d", diskSnapshotCycle);
			diskSnapshot = NULL;
//			 TODO: timeline loading failed, error state: rollback loading timeline, clear everything/reboot emulator
		}
		else
		{
			diskSnapshot = it->second;
			diskSnapshot->numLinkedChipsSnapshots++;
		}
	}
	
	LOGD("CStoredChipsSnapshot::RestoreFromFile cycle=%d diskSnapshot=%x", cycle, diskSnapshot);
}

CStoredInputEvent::CStoredInputEvent(CSnapshotsManager *manager, u32 frame, u64 cycle)
: CStoredSnapshot(manager, SNAPSHOT_TYPE_INPUT)
{
	this->Use(frame, cycle);
}

CStoredInputEvent::CStoredInputEvent(CSnapshotsManager *manager, CSlrFile *file)
: CStoredSnapshot(manager, SNAPSHOT_TYPE_INPUT)
{
	RestoreFromFile(file);
}

CSnapshotsManager::CSnapshotsManager(CDebugInterface *debugInterface)
{
	this->mutex = new CSlrMutex("CSnapshotsManager");
	this->debugInterface = debugInterface;
	this->viewTimeline = NULL;
	
	currentDiskSnapshot = NULL;
	snapshotToRestore = NULL;
	nextInputEvent = NULL;

	lastStoredFrame = 0;
	ResetLastStoredFrameCounter();
	
	// pause on frame num
	pauseNumCycle = -1;
	pauseNumFrame = -1;
	skipFrameRender = false;
	
	isPerformingSnapshotRestore = false;
//	skipSavingSnapshots = false;

	isStoreInputEventsEnabled = false;
	isReplayInputEventsEnabled = false;
	isOverwriteInputEventsEnabled = false;
	

	LOGD("CSnapshotsManager::CSnapshotsManager: snapshotsIntervalInFrames=%d snapshotsLimit=%d",
		 c64SettingsSnapshotsIntervalNumFrames, c64SettingsSnapshotsLimit);
}

CSnapshotsManager::~CSnapshotsManager()
{
}

void CSnapshotsManager::ResetLastStoredFrameCounter()
{
	lastStoredFrameCounter = c64SettingsSnapshotsIntervalNumFrames-1;
}

void CSnapshotsManager::CancelRestore()
{
	LockMutex();
	snapshotToRestore = NULL;
	isPerformingSnapshotRestore = false;
	pauseNumFrame = -1;
	pauseNumCycle = -1;
	skipFrameRender = false;
	UnlockMutex();

	debugInterface->RefreshScreenNoCallback();
}

// CheckSnapshotInterval should be run each frame from within code that can store snapshots,
// check if we are exactly in a new frame cycle should be done by the caller
bool CSnapshotsManager::CheckSnapshotInterval()
{
	LOGS("CSnapshotsManager::CheckSnapshotInterval: pauseNumCycle=%d currentCycle=%d", pauseNumCycle, debugInterface->GetMainCpuCycleCounter());
	
	if (snapshotToRestore || pauseNumCycle != -1) // || skipSavingSnapshots)
	{
		LOGS("snapshotToRestore=%x pauseNumCycle=%d", snapshotToRestore, pauseNumCycle);
		return false;
	}
	
	u32 currentFrame = debugInterface->GetEmulationFrameNumber();
	
	if (pauseNumFrame == currentFrame)
	{
		LOGD("CSnapshotsManager::CheckSnapshotInterval: pauseNumFrame == currentFrame");
		debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
		pauseNumFrame = -1;
		skipFrameRender = false;
		debugInterface->RefreshScreenNoCallback();
		isPerformingSnapshotRestore = false;
		return false;
	}

	if (c64SettingsSnapshotsRecordIsActive == false)
		return false;

	lastStoredFrameCounter++;
	
	// check if we should store snapshot (the frame interval is hit)
	if (lastStoredFrameCounter >= c64SettingsSnapshotsIntervalNumFrames)
	{
		LOGS("CSnapshotsManager::CheckSnapshotInterval: LockMutex");
		this->LockMutex();
		
		if (snapshotToRestore == NULL)
		{
			LOGS("CSnapshotsManager::CheckSnapshotInterval: snapshotToRestore=NULL");

			isPerformingSnapshotRestore = false;
			lastStoredFrameCounter = 0;
			
			u64 currentCycle = debugInterface->GetMainCpuCycleCounter();
			
			LOGS("CSnapshotsManager::CheckSnapshotInterval: store snapshot, currentFrame=%d, currentCycle=%d", currentFrame, currentCycle);
			lastStoredFrame = currentFrame;
			
			long t1 = SYS_GetCurrentTimeInMillis();
			
			// we are in CPU interrupt check, we can safely store snapshot now
			if (currentDiskSnapshot == NULL
				|| debugInterface->IsDriveDirtyForSnapshot())
			{
//				LOGD("....... create new disk snapshot");
				CStoredDiskSnapshot *diskSnapshot = GetNewDiskSnapshot(currentFrame, currentCycle);
				if (diskSnapshot == NULL)
				{
					LOGError("CSnapshotsManager::CheckSnapshotInterval failed");
					this->UnlockMutex();
					return false;
				}
				
				// store disk snapshot now, in a synced manner
				debugInterface->SaveDiskDataSnapshotSynced(diskSnapshot->byteBuffer);
				
				//	TODO: add int save_chips and store only disk GCR data      <- nope don't do this
				//int drive_snapshot_write_module(snapshot_t *s, int save_disks, int save_roms)
				
				debugInterface->ClearDriveDirtyForSnapshotFlag();
				
				currentDiskSnapshot = diskSnapshot;
				diskSnapshotsByFrame[currentFrame] = diskSnapshot;
				
				u64 cycle = debugInterface->GetMainCpuCycleCounter();
				diskSnapshotsByCycle[cycle] = diskSnapshot;
				
//				DebugPrintDiskSnapshots();
			}
			
			CStoredChipsSnapshot *chipSnapshot = GetNewChipSnapshot(currentFrame, currentCycle, currentDiskSnapshot);
			
			if (chipSnapshot == NULL)
			{
				LOGError("CSnapshotsManager::CheckSnapshotInterval failed");
				this->UnlockMutex();
				return false;
			}
			
			// store snapshot now, in a synced manner
			debugInterface->SaveChipsSnapshotSynced(chipSnapshot->byteBuffer);
			
			chipSnapshotsByFrame[currentFrame] = chipSnapshot;
			chipSnapshotsByCycle[chipSnapshot->cycle] = chipSnapshot;
			
//			DebugPrintChipsSnapshots();
			
			long t2 = SYS_GetCurrentTimeInMillis();
			
//			LOGD("CSnapshotsManager stored snapshot t=%d", t2-t1);
		}
		
		LOGS("CSnapshotsManager::CheckSnapshotInterval: UnlockMutex");

		this->UnlockMutex();
		
		return true;
	}
	
	// snapshot not stored
	return false;
}

void CSnapshotsManager::RestoreSnapshot(CStoredChipsSnapshot *snapshot)
{
	LOGS("CSnapshotsManager::RestoreSnapshot (set snapshotToRestore only)");
	this->LockMutex();

	snapshotToRestore = snapshot;
	
	this->UnlockMutex();

	LOGS("CSnapshotsManager::RestoreSnapshot done");
}

// The code below is run for each CPU cycle
bool CSnapshotsManager::CheckMainCpuCycle()
{
//	LOGD("CSnapshotsManager::CheckMainCpuCycle prev clk=%d now clk=%d", debugInterface->GetPreviousCpuInstructionCycleCounter(), debugInterface->GetMainCpuCycleCounter());
	
	if (snapshotToRestore)
		return false;
	
	if (pauseNumCycle == -1)
		return false;

	u64 currentCycle = debugInterface->GetMainCpuCycleCounter();

//	LOGD("previous_instr_maincpu_clk=%d pauseNumCycle=%d maincpu_clk=%d", debugInterface->GetPreviousCpuInstructionCycleCounter(), pauseNumCycle, currentCycle);
		
	if (pauseNumCycle <= currentCycle)
	{
		if (pauseNumCycle != currentCycle)
		{
			LOGError("Could not hit cycle %d, now we are at %d", pauseNumCycle, currentCycle);
		}
		
		LOGD("STOP: pauseNumCycle=%d currentCycle=%d frame=%d", pauseNumCycle, currentCycle, debugInterface->GetEmulationFrameNumber());
		debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
		pauseNumCycle = -1;
		pauseNumFrame = -1;
		skipFrameRender = false;
		debugInterface->RefreshScreenNoCallback();
		isPerformingSnapshotRestore = false;
		return true;
	}
	
	return false;
}

extern "C" {
	void c64d_reset_sound_clk();
}

bool CSnapshotsManager::CheckSnapshotRestore()
{
//	LOGD("CSnapshotsManager::CheckSnapshotRestore");
	this->LockMutex();

//	LOGD("CSnapshotsManager::CheckSnapshotRestore: after LockMutex");
	
	if (snapshotToRestore)
	{
		LOGS("!!!!!!!!!!!!!!!!!!!!!!!! RESTORING SNAPSHOT frame=%d cycle=%d", snapshotToRestore->frame, snapshotToRestore->cycle);
		
		gSoundEngine->LockMutex("CSnapshotsManager::CheckSnapshotRestore: restore snapshot");

		// restore disk
		CStoredDiskSnapshot *diskSnapshot = snapshotToRestore->diskSnapshot;
		
		// TODO: note, we *must* always have a disk snapshot. if disk snapshot is NULL that means there's a bug in storing code.
		// my observation is that this might happen when a regular snapshot is restored, then the disk snapshot is missing.
		// a solution for this is to completely skip emulator's snapshots and always use our own format.
		
		if (diskSnapshot != NULL)
		{
			LOGS("!!!!!!!!!!!!!!!!!!!!!!!!     -> diskSnapshot frame=%d", diskSnapshot->frame);
			debugInterface->LoadDiskDataSnapshotSynced(diskSnapshot->byteBuffer);
			
			// make all following chips snapshots use this diskSnapshot
			currentDiskSnapshot = diskSnapshot;
		}
				
		// restore chips
		debugInterface->LoadChipsSnapshotSynced(snapshotToRestore->byteBuffer);
		
		// clear all history events after restored cycle (mem write history, execute history)
		debugInterface->symbols->memory->ClearEventsAfterCycle(snapshotToRestore->cycle);

		snapshotToRestore = NULL;
		
		LOGS("!!!!!!!!!!!!!!!!!!!!!!!!     restored, currentFrame=%d pauseNumFrame=%d currentCycle=%d", debugInterface->GetEmulationFrameNumber(), pauseNumFrame, debugInterface->GetMainCpuCycleCounter());

		if (pauseNumFrame == -1 && pauseNumCycle == -1)
		{
			isPerformingSnapshotRestore = false;
		}

		// TODO: WTF c64d_reset_sound_clk?   generalize
		c64d_reset_sound_clk();
				
		gSoundEngine->UnlockMutex("CSnapshotsManager::CheckSnapshotRestore: restore snapshot");

		LOGS("CSnapshotsManager::CheckSnapshotRestore: UnlockMutex (1)");

		this->UnlockMutex();
		
		LOGS("!!!!!!!!!!!!!!!!!!!!!!!      SNAPSHOT RESTORED / FINISHED");
		return true;
	}
	
//	LOGD("CSnapshotsManager::CheckSnapshotRestore: UnlockMutex (2)");
	this->UnlockMutex();
	return false;
}

// @returns false=snapshot was not found, not possible to restore. cycleNum is optional, if -1 only frame will be searched.
bool CSnapshotsManager::RestoreSnapshotByFrame(int frame)
{
	if (frame < 0)
		return false;
	return RestoreSnapshotByFrame(frame, -1, debugInterface->GetDebugMode());
}

bool CSnapshotsManager::RestoreSnapshotByFrame(int frame, long cycleNum)
{
	return RestoreSnapshotByFrame(frame, cycleNum, debugInterface->GetDebugMode());
}

bool CSnapshotsManager::RestoreSnapshotByFrame(int frame, long cycleNum, u8 targetDebugMode)
{
	LOGD("RestoreSnapshotByFrame: frame=%d cycle=%d, LockMutex", frame, cycleNum);
	this->LockMutex();

	if (snapshotToRestore)
	{
		LOGS("RestoreSnapshotByFrame: UnlockMutex (1)");
		this->UnlockMutex();
		return false;
	}
	
	if (chipSnapshotsByFrame.empty())
	{
		LOGS("RestoreSnapshotByFrame: UnlockMutex (2)");
		this->UnlockMutex();
		return false;
	}

	LOGS("***** CSnapshotsManager::RestoreSnapshotByFrame frame=%d ***", frame);

	isPerformingSnapshotRestore = true;

	int minFrame, maxFrame;
	GetFramesLimits(&minFrame, &maxFrame);
	
	if (frame <= minFrame)
		frame = minFrame + 1;
	
	///////
	CStoredChipsSnapshot *nearestChipSnapshot = NULL;

	std::map<u32, CStoredChipsSnapshot *>::iterator it = chipSnapshotsByFrame.begin();

	//if (frame == 0)
	{
		nearestChipSnapshot = it->second;
	}
	
	// find nearest snapshot, just go through list now. TODO: optimize this
	int nearestChipSnapshotDist = INT_MAX;
	for( ; it != chipSnapshotsByFrame.end(); it++)
	{
		CStoredChipsSnapshot *chipSnapshot = it->second;
		
		// TODO BUG: when scrubbing to exact frame we need to be able to restore snapshot immediately, so we have to restore to previous one
		// (note, this does not affect C64). wrong, this may lock the emulation engine due to threads chase. workaround for now.
//#if defined(RUN_COMMODORE64)
//		if (chipSnapshot->frame <= frame)
//#else
		if (chipSnapshot->frame < frame)
//#endif
		{
			int d2 = frame - chipSnapshot->frame;
			if (d2 >= 0 && d2 < nearestChipSnapshotDist)
			{
				nearestChipSnapshot = chipSnapshot;
				nearestChipSnapshotDist = d2;
			}
		}
	}
	////////////
	
	if (nearestChipSnapshot != NULL)
	{
		LOGS(".... found snapshot frame %d", nearestChipSnapshot->frame);
		snapshotToRestore = nearestChipSnapshot;
	}
	
	lastStoredFrameCounter = 0;
	
	// should we pause after restoring state to cycle?
	if (pauseNumFrame != -1 || targetDebugMode == DEBUGGER_MODE_PAUSED)
	{
		LOGS("nearestChipSnapshot->frame=%d frame=%d", nearestChipSnapshot->frame, frame);
		if (nearestChipSnapshot->frame < frame)
		{
			LOGS("......... nearestChipSnapshot->frame=%d but we are looking for %d", nearestChipSnapshot->frame, frame);
			
			if (cycleNum == -1)
			{
				// do not go to cycle
				pauseNumCycle = -1;
				pauseNumFrame = frame;
			}
			else
			{
				// go to cycle
				pauseNumCycle = cycleNum;
				pauseNumFrame = -1;
			}
			
			debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
			
			CStoredChipsSnapshot *lastChipsSnapshot = (--chipSnapshotsByFrame.end())->second;
			if (lastChipsSnapshot->frame + c64SettingsSnapshotsIntervalNumFrames < frame)
			{
				// do not skip render
			}
			else
			{
				skipFrameRender = true;
			}
		}
		else
		{
			LOGS("... found frame, going to cycle cycleNum=%d", cycleNum);
			if (cycleNum == -1)
			{
//#if defined(RUN_COMMODORE64)
//				// TODO BUG: we can't restore exact frame, the code below restores synchronously, although we are paused we are not sure if we are in a situation that we can restore snapshot synchronously
//				skipFrameRender = false;
//				pauseNumCycle = -1;
//				pauseNumFrame = -1;
//				// restore snapshot immediately
//				this->CheckSnapshotRestore();
//				debugInterface->SetDebugMode(DEBUGGER_MODE_PAUSED);
//				isPerformingSnapshotRestore = false;
//#else
				//
				// just run one cycle, we are already where we should be
				skipFrameRender = true;
				pauseNumCycle = -1;
				
				// TODO maybe a bug: note this may not restore correctly and sometimes may not stop at all when debug interface is doing messy things  on some emu interfces, also above the cycle may be not right sometimes in some emus
				pauseNumFrame = frame;
				debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
//#endif
			}
			else
			{
				skipFrameRender = true;
				pauseNumCycle = cycleNum;
				pauseNumFrame = -1;
				debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
			}
		}
	}
	
	LOGS("RestoreSnapshotByFrame: UnlockMutex (3)");
	this->UnlockMutex();

	return true;
}

bool CSnapshotsManager::IsPerformingSnapshotRestore()
{
	// ??
	if (this->isPerformingSnapshotRestore && debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
	{
		LOGError("isPerformingSnapshotRestore && debugMode=%d, frame=%d pauseNumFrame=%d pauseNumCycle=%d", debugInterface->GetDebugMode(),
				 debugInterface->GetEmulationFrameNumber(), pauseNumFrame, pauseNumCycle);
		
		if (pauseNumFrame != -1 && pauseNumFrame < debugInterface->GetEmulationFrameNumber())
		{
			debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
		if (pauseNumCycle != -1 && pauseNumCycle < debugInterface->GetMainCpuCycleCounter())
		{
			debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
	}
	
	return this->isPerformingSnapshotRestore;
}

// this is called by CDebugInterface when new input event is fired
// result is CByteBuffer that can be filled with events and later replayed by CDebugInterface at a given cycle
// note: this creates/adds the CStoredInputEvent, so CDebugInterface will fill the required data after it is added
CByteBuffer *CSnapshotsManager::StoreNewInputEventsSnapshotAtCurrentCycle()
{
	if (isStoreInputEventsEnabled == false)
		return NULL;
	
	u64 currentCycle = debugInterface->GetMainCpuCycleCounter();
	
	LOGD("StoreNewInputEventsSnapshotAtCurrentCycle: cycle=%d", currentCycle);
	
	// check if there's event buffer already associated to this cycle
	std::map<u64, CStoredInputEvent *>::iterator it = inputEventsByCycle.find(currentCycle);
	
	if (it != inputEventsByCycle.end())
	{
		CStoredInputEvent *inputEvent = it->second;
		
		// Note: we are not clearing this, as there may be multiple events in the buffer stored at the same cycle
//		inputEvent->byteBuffer->Clear();
				
		return inputEvent->byteBuffer;
	}
	
	u32 currentFrame = debugInterface->GetEmulationFrameNumber();
	
	CStoredInputEvent *inputEvent = GetNewInputEventSnapshot(currentFrame, currentCycle);
	
	inputEventsByCycle[currentCycle] = inputEvent;
	
	return inputEvent->byteBuffer;
}

// checks if at current CPU cycle there are events to be replayed, this is called by CheckMainCpuCycle
bool CSnapshotsManager::CheckInputEventsAtCurrentCycle()
{
	u64 cycle = debugInterface->GetMainCpuCycleCounter();
	
	if (isOverwriteInputEventsEnabled == true)
	{
		// TODO: note the OPTIMIZE below
		
		// TODO: add virtual method in CDebugInterface to override this and allow f.e. removal only of one controller from buffer
		//       (bool return if we can add to inputEventsToReuse)
		std::map<u64, CStoredInputEvent *>::iterator it = inputEventsByCycle.find(cycle);
		
		if (it != inputEventsByCycle.end())
		{
			CStoredInputEvent *inputEvent = it->second;
			inputEventsByCycle.erase(it);
			inputEvent->Clear();
			inputEventsToReuse.push_back(inputEvent);
		}
		
		return false;
	}
	
	if (isReplayInputEventsEnabled == false)
		return false;
		
//	LOGD("CheckInputEventsAtCurrentCycle: cycle=%d", cycle);

	// TODO: OPTIMIZE ME get lower_bound on scrub and store/use here nextInputEvent
	//       (iterator to upcoming event, so instead of find check just the cycle number)
	
//	if (inputEventsByCycle.empty() == false)
//		DebugPrintInputEventsSnapshots();
	
	// check if there's event buffer already associated to this cycle
	std::map<u64, CStoredInputEvent *>::iterator it = inputEventsByCycle.find(cycle);
	
	if (it != inputEventsByCycle.end())
	{
		LOGD("ReplayInputEventsFromSnapshotsManager: cycle=%d", cycle);
		CStoredInputEvent *inputEvents = it->second;
		inputEvents->byteBuffer->Rewind();
		debugInterface->ReplayInputEventsFromSnapshotsManager(inputEvents->byteBuffer);
		return true;
	}
	
	return false;
}

CStoredChipsSnapshot *CSnapshotsManager::GetNewChipSnapshot(u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot)
{
	LOGS("*************************** CSnapshotsManager::GetNewChipSnapshot: frame=%d        << CHIPS", frame);

//	LOGD("chipSnapshotsByFrame=%d chipSnapshotsLimit=%d chipsSnapshotsToReuse=%d", chipSnapshotsByFrame.size(), c64SettingsSnapshotsLimit, chipsSnapshotsToReuse.size());
//	LOGD("diskSnapshotsByFrame=%d diskSnapshotsToReuse=%d", diskSnapshotsByFrame.size(), diskSnapshotsToReuse.size());
	
	CStoredChipsSnapshot *chipSnapshot = NULL;
	
	std::map<u32, CStoredChipsSnapshot *>::iterator it = chipSnapshotsByFrame.find(frame);

	if (it == chipSnapshotsByFrame.end())
	{
		if (!chipsSnapshotsToReuse.empty())
		{
//			LOGD("...reuse CStoredChipsSnapshot chipsSnapshotsToReuse.size=%d", chipsSnapshotsToReuse.size());
			chipSnapshot = chipsSnapshotsToReuse.front();
			chipsSnapshotsToReuse.pop_front();
			chipSnapshot->Clear();
			chipSnapshot->Use(frame, cycle, diskSnapshot);

		}
		else
		{
			int f = chipSnapshotsByFrame.size();
			
			if (f < c64SettingsSnapshotsLimit)
			{
	//			LOGD("...create new CStoredChipsSnapshot");
				// create new
				chipSnapshot = new CStoredChipsSnapshot(this, frame, cycle, diskSnapshot);
				if (chipSnapshot == NULL)
				{
					LOGError("CSnapshotsManager::GetNewChipSnapshot: failed");
					return NULL;
				}
			}
			else
			{
				it = chipSnapshotsByFrame.begin();
				chipSnapshot = it->second;
				chipSnapshotsByFrame.erase(chipSnapshot->frame);
				chipSnapshotsByCycle.erase(chipSnapshot->cycle);
				chipSnapshot->Clear();
				chipSnapshot->Use(frame, cycle, diskSnapshot);
			}
		}

	}
	else
	{
		chipSnapshot = it->second;
		chipSnapshotsByFrame.erase(chipSnapshot->frame);
		chipSnapshotsByCycle.erase(chipSnapshot->cycle);
		chipSnapshot->Clear();
		chipSnapshot->Use(frame, cycle, diskSnapshot);
	}

//	LOGD("CSnapshotsManager::GetNewChipSnapshot: done");
	return chipSnapshot;
}

CStoredChipsSnapshot *CSnapshotsManager::GetNewChipSnapshot(CSlrFile *file)
{
	LOGS("*************************** CSnapshotsManager::GetNewChipSnapshot: file        << CHIPS");

//	LOGD("chipSnapshotsByFrame=%d chipSnapshotsLimit=%d chipsSnapshotsToReuse=%d", chipSnapshotsByFrame.size(), c64SettingsSnapshotsLimit, chipsSnapshotsToReuse.size());
//	LOGD("diskSnapshotsByFrame=%d diskSnapshotsToReuse=%d", diskSnapshotsByFrame.size(), diskSnapshotsToReuse.size());
	
	// TODO: CSnapshotsManager::GetNewChipSnapshot we do not obey # of snapshots limits rule when reading timeline from file for now, add settings switch and implement the limit here
	CStoredChipsSnapshot *chipSnapshot = NULL;
	
	if (chipsSnapshotsToReuse.empty())
	{
		chipSnapshot = new CStoredChipsSnapshot(this, file);
	}
	else
	{
		chipSnapshot = chipsSnapshotsToReuse.front();
		chipsSnapshotsToReuse.pop_front();
		chipSnapshot->Clear();
		chipSnapshot->RestoreFromFile(file);
	}

//	LOGD("CSnapshotsManager::GetNewChipSnapshot: done");
	return chipSnapshot;
}

CStoredDiskSnapshot *CSnapshotsManager::GetNewDiskSnapshot(u32 frame, u64 cycle)
{
	LOGS("*************************** CSnapshotsManager::GetNewDiskSnapshot: frame=%d        << DISK", frame);
//	LOGD("diskSnapshotsToReuse=%d", diskSnapshotsToReuse.size());
	
	CStoredDiskSnapshot *diskSnapshot = NULL;
	
	if (diskSnapshotsToReuse.size() == 0)
	{
//		LOGD("...create new CStoredDiskSnapshot");
		// create new
		diskSnapshot = new CStoredDiskSnapshot(this, frame, cycle);
		if (diskSnapshot == NULL)
		{
			LOGError("CSnapshotsManager::GetNewChipSnapshot: failed");
			return NULL;
		}
	}
	else
	{
//		LOGD("...reuse CStoredDiskSnapshot");
		diskSnapshot = diskSnapshotsToReuse.front();
		diskSnapshotsToReuse.pop_front();
		diskSnapshot->Clear();
		diskSnapshot->Use(frame, cycle);
		
		LOGS("...reuse diskSnapshot data=%x", diskSnapshot->byteBuffer->data);
	}
	
//	LOGD("CSnapshotsManager::GetNewDiskSnapshot: done");
	return diskSnapshot;
}

CStoredDiskSnapshot *CSnapshotsManager::GetNewDiskSnapshot(CSlrFile *file)
{
	LOGS("*************************** CSnapshotsManager::GetNewDiskSnapshot: file        << DISK");
//	LOGD("diskSnapshotsToReuse=%d", diskSnapshotsToReuse.size());
	
	CStoredDiskSnapshot *diskSnapshot = NULL;
	
	if (diskSnapshotsToReuse.empty())
	{
//		LOGD("...create new CStoredDiskSnapshot");
		// create new
		diskSnapshot = new CStoredDiskSnapshot(this, file);
		if (diskSnapshot == NULL)
		{
			LOGError("CSnapshotsManager::GetNewChipSnapshot: failed");
			return NULL;
		}
	}
	else
	{
//		LOGD("...reuse CStoredDiskSnapshot");
		diskSnapshot = diskSnapshotsToReuse.front();
		
		diskSnapshotsToReuse.pop_front();

		diskSnapshot->Clear();
		diskSnapshot->RestoreFromFile(file);
		
		LOGS("...reuse diskSnapshot data=%x", diskSnapshot->byteBuffer->data);
	}
	
//	LOGD("CSnapshotsManager::GetNewDiskSnapshot: done");
	return diskSnapshot;
}

CStoredInputEvent *CSnapshotsManager::GetNewInputEventSnapshot(u32 frame, u64 cycle)
{
	LOGS("*************************** CSnapshotsManager::GetNewInputEventSnapshot: frame=%d cycle=%d       << INPUT", frame, cycle);
	
//	LOGD("inputEventsByCycle=%d c64SettingsSnapshotsLimit=%d inputEventsToReuse=%d", inputEventsByCycle.size(), c64SettingsSnapshotsLimit, inputEventsToReuse.size());
	
	CStoredInputEvent *inputEventsSnapshot = NULL;
	
	std::map<u64, CStoredInputEvent *>::iterator it = inputEventsByCycle.find(cycle);

	if (it == inputEventsByCycle.end())
	{
		int f = inputEventsByCycle.size();
		
		if (f < c64SettingsSnapshotsLimit)
		{
//			LOGD("...create new CStoredInputEvent");
			// create new
			inputEventsSnapshot = new CStoredInputEvent(this, frame, cycle);
			if (inputEventsSnapshot == NULL)
			{
				LOGError("CSnapshotsManager::GetNewInputEventSnapshot: failed");
				return NULL;
			}
		}
		else
		{
//			LOGD("...reuse CStoredInputEvent inputEventsToReuse.size=%d", inputEventsToReuse.size());
			int s = inputEventsToReuse.size();

//			LOGD("s=%d", s);
			
			if (s == 0)
			{
				it = inputEventsByCycle.begin();
				inputEventsSnapshot = it->second;
				inputEventsByCycle.erase(inputEventsSnapshot->frame);
			}
			else
			{
				inputEventsSnapshot = inputEventsToReuse.front();
				inputEventsToReuse.pop_front();
			}
			inputEventsSnapshot->Clear();
			inputEventsSnapshot->Use(frame, cycle);
		}
	}
	else
	{
		inputEventsSnapshot = it->second;
		inputEventsByCycle.erase(inputEventsSnapshot->frame);
		inputEventsSnapshot->Clear();
		inputEventsSnapshot->Use(frame, cycle);
	}

//	LOGD("CSnapshotsManager::GetNewInputEventSnapshot: done");
	return inputEventsSnapshot;
}

CStoredInputEvent *CSnapshotsManager::GetNewInputEventSnapshot(CSlrFile *file)
{
	LOGS("*************************** CSnapshotsManager::GetNewInputEventSnapshot: file        << CHIPS");

	// TODO: CSnapshotsManager::GetNewInputEventSnapshot we do not obey # of snapshots limits rule when reading timeline from file for now, add settings switch and implement the limit here
	CStoredInputEvent *inputEvent = NULL;
	
	if (inputEventsToReuse.empty())
	{
		inputEvent = new CStoredInputEvent(this, file);
	}
	else
	{
		inputEvent = inputEventsToReuse.front();
		inputEventsToReuse.pop_front();
		inputEvent->Clear();
		inputEvent->RestoreFromFile(file);
	}

//	LOGD("CSnapshotsManager::GetNewInputEventSnapshot: done");
	return inputEvent;
}

///

void CSnapshotsManager::ClearSnapshotsHistory()
{
	// this will completely remove all history, used when new PRG is loaded or cart is inserted/detached
	LOGD("CSnapshotsManager::ClearSnapshotsHistory");
	
	this->LockMutex();
//	LOGD("CSnapshotsManager::ClearSnapshotsHistory: locked");
	
	snapshotToRestore = NULL;
	currentDiskSnapshot = NULL;
	
	for (std::map<u32, CStoredDiskSnapshot *>::iterator it = diskSnapshotsByFrame.begin(); it != diskSnapshotsByFrame.end(); it++)
	{
		CStoredDiskSnapshot *diskSnapshot = it->second;
		diskSnapshot->numLinkedChipsSnapshots = 0;
		diskSnapshotsToReuse.push_back(diskSnapshot);
	}
	diskSnapshotsByFrame.clear();
	diskSnapshotsByCycle.clear();

	for (std::map<u32, CStoredChipsSnapshot *>::iterator it = chipSnapshotsByFrame.begin(); it != chipSnapshotsByFrame.end(); it++)
	{
		CStoredChipsSnapshot *chipsSnapshot = it->second;
		chipsSnapshot->diskSnapshot = NULL;
		chipsSnapshot->Clear();
		chipsSnapshotsToReuse.push_back(chipsSnapshot);
	}
	chipSnapshotsByFrame.clear();
	chipSnapshotsByCycle.clear();

	for (std::map<u64, CStoredInputEvent *>::iterator it = inputEventsByCycle.begin(); it != inputEventsByCycle.end(); it++)
	{
		CStoredInputEvent *inputEvent = it->second;
		inputEvent->Clear();
		inputEventsToReuse.push_back(inputEvent);
	}
	inputEventsByCycle.clear();
	
	//
	ResetLastStoredFrameCounter();
	
	pauseNumCycle = -1;
	pauseNumFrame = -1;
	skipFrameRender = false;

//	LOGD("CSnapshotsManager::ClearSnapshotsHistory: unlock");
	this->UnlockMutex();
}

void CSnapshotsManager::RestoreSnapshotByNumFramesOffset(int numFramesOffset)
{
	LOGD("CSnapshotsManager::RestoreSnapshotByNumFramesOffset: numFramesOffset=%d", numFramesOffset);
	this->LockMutex();
	
	LOGD("CSnapshotsManager::RestoreSnapshotByNumFramesOffset: locked");
	int currentFrame = debugInterface->GetEmulationFrameNumber();
	int restoreToFrame = currentFrame + numFramesOffset;
	
	int minFrame, maxFrame;
	GetFramesLimits(&minFrame, &maxFrame);
	
	if (restoreToFrame <= minFrame)
		restoreToFrame = minFrame + 1;
	
	LOGD(">>>>>>>>>................ currentFrame=%d restoreToFrame=%d", currentFrame, restoreToFrame);
	debugInterface->snapshotsManager->RestoreSnapshotByFrame(restoreToFrame, -1);

	LOGD("CSnapshotsManager::RestoreSnapshotByNumFramesOffset: unlock");
	this->UnlockMutex();
}

void CSnapshotsManager::RestoreSnapshotBackstepInstruction()
{
	LOGS("CSnapshotsManager::RestoreSnapshotBackstepInstruction");
	this->LockMutex();
	LOGS("CSnapshotsManager::RestoreSnapshotBackstepInstruction: locked");
	
//	int currentFrame = debugInterface->GetEmulationFrameNumber();
//	int restoreToFrame = currentFrame-1;
//
//	int minFrame, maxFrame;
//	GetFramesLimits(&minFrame, &maxFrame);
//
//	if (restoreToFrame <= minFrame)
//	{
//		LOGS("CSnapshotsManager::RestoreSnapshotBackstepInstruction: unlock");
//		this->UnlockMutex();
//		return;
//	}
	
//	LOGD(">>>>>>>>>................ currentFrame=%d restoreToFrame=%d", currentFrame, restoreToFrame);

	unsigned int previousInstructionClk = debugInterface->GetPreviousCpuInstructionCycleCounter();
	
	LOGD(">>>>>>>>>................ previousInstructionClk=%d", previousInstructionClk);
	
//	debugInterface->snapshotsManager->RestoreSnapshotByFrame(restoreToFrame, previousInstructionClk);
	debugInterface->snapshotsManager->RestoreSnapshotByCycle(previousInstructionClk);
	
	LOGS("CSnapshotsManager::RestoreSnapshotBackstepInstruction: unlock");
	this->UnlockMutex();
}

bool CSnapshotsManager::SkipRefreshOfVideoFrame()
{
//	if (skipFrameRender)
//	{
//		int targetFrame = debugInterface->GetEmulationFrameNumber()+1;
//		if (pauseNumFrame == targetFrame)
//		{
////			LOGD("SkipRefreshOfVideoFrame: FALSE: pauseNumFrame=%d targetFrame=%d", pauseNumFrame, targetFrame);
//			return false;
//		}
//	}
//	LOGD("SkipRefreshOfVideoFrame: %s", STRBOOL(skipFrameRender));
	return skipFrameRender;
}

void CSnapshotsManager::GetFramesLimits(int *minFrame, int *maxFrame)
{
	this->LockMutex();
	
	if (chipSnapshotsByFrame.size() < 1)
	{
		*minFrame = 0;
		*maxFrame = 0;
		this->UnlockMutex();
		return;
	}
	
	std::map<u32, CStoredChipsSnapshot *>::iterator itFirst = chipSnapshotsByFrame.begin();
	*minFrame = itFirst->first;

	std::map<u32, CStoredChipsSnapshot *>::iterator itLast = --chipSnapshotsByFrame.end();
	*maxFrame = itLast->first;
	
	this->UnlockMutex();
}


void CSnapshotsManager::SetRecordingIsActive(bool isActive)
{
	debugInterface->LockMutex();

//	this->ClearSnapshotsHistory();
	c64SettingsSnapshotsRecordIsActive = isActive;

	debugInterface->UnlockMutex();
}

void CSnapshotsManager::SetRecordingStoreInterval(int recordingInterval)
{
	if (recordingInterval < 1)
	{
		recordingInterval = 1;
	}
	debugInterface->LockMutex();
	c64SettingsSnapshotsIntervalNumFrames = recordingInterval;
	debugInterface->UnlockMutex();
}

void CSnapshotsManager::SetRecordingLimit(int recordingLimit)
{
	if (recordingLimit < 2)
	{
		recordingLimit = 2;
	}
	debugInterface->LockMutex();
	c64SettingsSnapshotsLimit = recordingLimit;
	debugInterface->UnlockMutex();
}

bool CSnapshotsManager::RestoreSnapshotByCycle(u64 cycle)
{
	return RestoreSnapshotByCycle(cycle, debugInterface->GetDebugMode());
}

bool CSnapshotsManager::RestoreSnapshotByCycle(u64 cycle, u8 targetDebugMode)
{
	LOGD("CSnapshotsManager::RestoreSnapshotByCycle: cycle=%d", cycle);
	// TODO: fixme and reuse code considering that we have found the snapshot, thus pass it further
	//       the RestoreSnapshotByFrame searches this again, so it is twice.
	
	// to have this working we need to find cycle within frame, i.e. iterate over frames to find cycle
	this->LockMutex();

	if (snapshotToRestore)
	{
		LOGS("RestoreSnapshotByFrame: UnlockMutex (1)");
		this->UnlockMutex();
		return false;
	}
	
	if (chipSnapshotsByFrame.empty())
	{
		LOGS("RestoreSnapshotByFrame: UnlockMutex (2)");
		this->UnlockMutex();
		return false;
	}

	LOGD("***** CSnapshotsManager::RestoreSnapshotByCycle cycle=%d ***", cycle);

	std::map<u32, CStoredChipsSnapshot *>::iterator it = chipSnapshotsByFrame.begin();
	CStoredChipsSnapshot *nearestChipSnapshot = NULL;

	//if (frame == 0)
	{
		nearestChipSnapshot = it->second;
	}
	
	// find nearest snapshot frame, just go through list now. TODO: optimize this
	// chipSnapshotsByFrame.lower_bound(<#const key_type &__k#>)
	for( ; it != chipSnapshotsByFrame.end(); it++)
	{
		CStoredChipsSnapshot *chipSnapshot = it->second;
		
		if (chipSnapshot->cycle < cycle)
		{
			nearestChipSnapshot = chipSnapshot;
		}
	}
	
	if (nearestChipSnapshot == NULL)
	{
		this->UnlockMutex();
		return false;
	}

	LOGD(".... FOUND nearestChipSnapshot->cycle=%d, run to cycle=%d", nearestChipSnapshot->cycle, cycle);

	LOGD("RestoreSnapshotByCycle: OK run RestoreSnapshotByFrame frame=%d cycle=%d", nearestChipSnapshot->frame, cycle);
	bool ret = RestoreSnapshotByFrame(nearestChipSnapshot->frame-2, cycle, targetDebugMode);
	
	if (targetDebugMode == DEBUGGER_MODE_RUNNING)
	{
		debugInterface->SetDebugMode(targetDebugMode);
	}

	this->UnlockMutex();
	
	return ret;
}

/*

// SPAGHETTI:

bool CSnapshotsManager::RestoreDiskSnapshotByCycle(u64 cycle)
{
	LOGD("CSnapshotsManager::RestoreDiskSnapshotByCycle: cycle=%d", cycle);
	// TODO: fixme and reuse code considering that we have found the snapshot, thus pass it further
	//       the RestoreSnapshotByFrame searches this again, so it is twice.
	
	// to have this working we need to find cycle within frame, i.e. iterate over frames to find cycle
	this->LockMutex();

	if (snapshotToRestore)
	{
		LOGS("RestoreSnapshotByFrame: UnlockMutex (1)");
		this->UnlockMutex();
		return false;
	}
	
	if (diskSnapshotsByFrame.empty())
	{
		LOGS("RestoreSnapshotByFrame: UnlockMutex (2)");
		this->UnlockMutex();
		return false;
	}

	LOGD("***** CSnapshotsManager::RestoreSnapshotByCycle cycle=%d ***", cycle);

	std::map<u32, CStoredDiskSnapshot *>::iterator it = diskSnapshotsByFrame.begin();
	CStoredDiskSnapshot *nearestDiskSnapshot = NULL;

	//if (frame == 0)
	{
		nearestDiskSnapshot = it->second;
	}
	
	// find nearest disk snapshot frame, just go through list now. TODO: optimize this
	// diskSnapshotsByFrame.lower_bound(<#const key_type &__k#>)
	for( ; it != diskSnapshotsByFrame.end(); it++)
	{
		CStoredDisksSnapshot *diskSnapshot = it->second;
		
		if (diskSnapshot->cycle < cycle)
		{
			nearestDiskSnapshot = diskSnapshot;
		}
	}
	
	if (nearestDiskSnapshot == NULL)
	{
		this->UnlockMutex();
		return false;
	}

	LOGD(".... FOUND nearestDiskSnapshot->cycle=%d, run to cycle=%d", nearestDiskSnapshot->cycle, cycle);

	LOGD("RestoreDiskSnapshotByCycle: OK run RestoreDiskSnapshotByFrame frame=%d cycle=%d", nearestDiskSnapshot->frame, cycle);
	bool ret = RestoreDiskSnapshotByFrame(nearestDiskSnapshot->frame-2, cycle);
	
	this->UnlockMutex();
	
	return ret;
}
*/

//
void CSnapshotsManager::DeleteAllPools()
{
	LOGD("CSnapshotsManager::DeleteAllPools");
	
	this->LockMutex();
	
	while(!chipsSnapshotsToReuse.empty())
	{
		CStoredChipsSnapshot *s = chipsSnapshotsToReuse.front();
		chipsSnapshotsToReuse.pop_front();
		delete s;
	}

	while(!diskSnapshotsToReuse.empty())
	{
		CStoredDiskSnapshot *s = diskSnapshotsToReuse.front();
		diskSnapshotsToReuse.pop_front();
		delete s;
	}

	while(!inputEventsToReuse.empty())
	{
		CStoredInputEvent *s = inputEventsToReuse.front();
		inputEventsToReuse.pop_front();
		delete s;
	}

	this->UnlockMutex();
}


// RetroDebuggerTimeLine
float CSnapshotsManager::GetGuiViewProgressBarWindowValue(void *userData)
{
	return progressStoreOrRestore;
}

void CSnapshotsManager::StoreTimelineToFile(CSlrString *timelineFilePath)
{
	const char *cFilePath = timelineFilePath->GetStdASCII();
	LOGM("CSnapshotsManager::StoreTimelineToFile: %s", cFilePath);
	
	progressStoreOrRestore = 0.0f;
	u8 prevDebugMode = debugInterface->SetDebugModeBlockedWait(DEBUGGER_MODE_PAUSED);
	
	CSlrFileFromOS *file = new CSlrFileFromOS(timelineFilePath, SLR_FILE_MODE_WRITE);
	file->WriteU32(RDTL_MAGIC);
	file->WriteU16(RDTL_VERSION);
	
	// timeline settings
	file->WriteU8(debugInterface->GetEmulatorType());
	file->WriteU32(debugInterface->GetEmulationFrameNumber());
	file->WriteU64(debugInterface->GetMainCpuCycleCounter());
	
	// number of snapshots/events
	file->WriteU32(diskSnapshotsByCycle.size());
	file->WriteU32(chipSnapshotsByCycle.size());
	file->WriteU32(inputEventsByCycle.size());
	
	progressNumSnapshots = chipSnapshotsByCycle.size();
	progressCurrentSnapshot = 0;

	// show progress bar
	CSlrString *fileName = timelineFilePath->GetFileNameComponentFromPath();
	char *cFileName = fileName->GetStdASCII();

	char *progressBarWindowTitle = SYS_GetCharBuf();
	sprintf(progressBarWindowTitle, "Save %s Timeline: %s", debugInterface->GetPlatformNameString(), cFileName);
	viewC64->viewProgressBarWindow->ShowProgressBar(progressBarWindowTitle, debugInterface->snapshotsManager);

	STRFREE(cFileName);
	delete fileName;
	
	// store timeline snapshots as compressed zlib stream
	CSlrFileZlib *zlibFile = new CSlrFileZlib(file, c64SettingsTimelineSaveZlibCompressionLevel);

	StoreTimelineSnapshotsToFile(zlibFile);
	
	delete zlibFile;	
	delete file;
	
	LOGM("CSnapshotsManager::StoreTimelineToFile done: %s", cFilePath);
	STRFREE(cFilePath);

	// hide progress bar
	viewC64->viewProgressBarWindow->HideProgressBar();

	debugInterface->SetDebugMode(prevDebugMode);
	
	SYS_ReleaseCharBuf(progressBarWindowTitle);
}

bool CSnapshotsManager::RestoreTimelineFromFile(CSlrString *timelineFilePath)
{
	// TODO: this has UI logic and should not be a part of CSnapshotsManager. We need to create 'file opener' class for all file types and derive this loader, this is temporary here and should be immediately removed from here.
	
	const char *cFilePath = timelineFilePath->GetStdASCII();
	LOGM("CViewC64::LoadTimeline: %s", cFilePath);

	progressStoreOrRestore = 0.0f;

	CSlrFileFromOS *file = new CSlrFileFromOS(timelineFilePath, SLR_FILE_MODE_READ);
	
	u32 magic = file->ReadU32();
	if (magic != RDTL_MAGIC)
	{
		LOGError("CViewC64::LoadTimeline: magic not correct %08x should be %08x", magic, RDTL_MAGIC);
		guiMain->ShowMessageBox("Timeline failed", "Timeline file is broken, magic not correct");
		delete file;
		return false;
	}
	
	u16 version = file->ReadU16();
	if (version != RDTL_VERSION)
	{
		LOGError("CViewC64::LoadTimeline: version not correct %04x should be %04x", version, RDTL_VERSION);
		guiMain->ShowMessageBox("Timeline failed", "Timeline file version is unknown");
		delete file;
		return false;
	}

	u8 emulatorType = file->ReadU8();
	CDebugInterface *debugInterface = viewC64->GetDebugInterface(emulatorType);
	if (debugInterface == NULL)
	{
		LOGError("CViewC64::LoadTimeline: emulator type=%02x unknown", emulatorType);
		guiMain->ShowMessageBox("Timeline failed", "Timeline file emulator type is unknown");
		delete file;
		return false;
	}

	// show progress bar
	CSlrString *fileName = timelineFilePath->GetFileNameComponentFromPath();
	char *cFileName = fileName->GetStdASCII();
	
	char *progressBarWindowTitle = SYS_GetCharBuf();
	sprintf(progressBarWindowTitle, "Load %s Timeline: %s", debugInterface->GetPlatformNameString(), cFileName);
	viewC64->viewProgressBarWindow->ShowProgressBar(progressBarWindowTitle, debugInterface->snapshotsManager);
	
	STRFREE(cFileName);
	delete fileName;
	
	//
	u32 readTimelineFrameNumber = file->ReadU32();
	u64 readTimelineCpuCycle = file->ReadU64();
	
	u32 numDiskSnapshots = file->ReadU32();
	u32 numChipsSnapshots = file->ReadU32();
	u32 numInputEvents = file->ReadU32();
		
	progressNumSnapshots = numChipsSnapshots;
	progressCurrentSnapshot = 0;
	
	// restore timeline snapshots from compressed zlib stream
	CSlrFileZlib *zlibFile = new CSlrFileZlib(file);

	u8 prevDebugMode = debugInterface->SetDebugModeBlockedWait(DEBUGGER_MODE_PAUSED);
	bool ret = debugInterface->snapshotsManager->RestoreTimelineSnapshotsFromFile(zlibFile);

	delete zlibFile;
	delete file;
	
	LOGM("CViewC64::LoadTimeline done: %s", cFilePath);
	delete [] cFilePath;
	
	LOGM("CViewC64::LoadTimeline: Rewind emulation to timeline cycle=%d", readTimelineCpuCycle);
	
	// hide progress bar
	viewC64->viewProgressBarWindow->HideProgressBar();

	debugInterface->snapshotsManager->RestoreSnapshotByCycle(readTimelineCpuCycle, prevDebugMode);
		
	SYS_ReleaseCharBuf(progressBarWindowTitle);
	return ret;
}

void CSnapshotsManager::StoreTimelineSnapshotsToFile(CSlrFile *file)
{
	if (diskSnapshotsByCycle.empty() || chipSnapshotsByCycle.empty())
	{
		LOGError("CSnapshotsManager::StoreTimelineToFile: diskSnapshots.size=%d chipSnapshots.size=%d", diskSnapshotsByCycle.size(), chipSnapshotsByCycle.size());

		// no snapshots, put end of snapshots marker
		file->WriteU8(SNAPSHOT_TYPE_NONE);
		return;
	}
	
	// write 'stream' of:
	// - disk snapshots
	// - chips snapshots
	// - input events
	// snapshots should be ordered by cycle in the above order, disk first, then chips that reference that disk snapshot, then events to chips snapshot
	// example:
	//  disk chips chips event chips disk chips chips event event chips chips event event event chips disk chips disk chips event disk chips etc
	
	itStoringDiskSnapshots = diskSnapshotsByCycle.begin();
	itStoringChipsSnapshots = chipSnapshotsByCycle.begin();
	itStoringInputEvents = inputEventsByCycle.begin();
	
	CStoredDiskSnapshot *diskSnapshot = itStoringDiskSnapshots->second;

	while(itStoringDiskSnapshots != diskSnapshotsByCycle.end())
	{
		LOGD(">> store disk snapshot %d", diskSnapshot->cycle);
		diskSnapshot->StoreToFile(file);
		
		itStoringDiskSnapshots++;
		
		if (itStoringDiskSnapshots == diskSnapshotsByCycle.end())
		{
			// store remaining chips snapshots
			StoreChipsSnapshotsAndInputEventsTillCycle(file, ULONG_MAX);
			break;
		}
		
		CStoredDiskSnapshot *nextDiskSnapshot = itStoringDiskSnapshots->second;
		u64 nextCycle = nextDiskSnapshot->cycle;
		StoreChipsSnapshotsAndInputEventsTillCycle(file, nextCycle);
		
		diskSnapshot = nextDiskSnapshot;
	}

	// end of snapshots marker
	file->WriteU8(SNAPSHOT_TYPE_NONE);

	LOGD("CSnapshotsManager::StoreTimelineToFile: done");
}

void CSnapshotsManager::StoreChipsSnapshotsAndInputEventsTillCycle(CSlrFile *file, u64 maxCycle)
{
	LOGD("StoreChipsSnapshotsAndInputEventsTillCycle cycle=%d", maxCycle);
	u64 chipsCycle = ULONG_MAX;
	u64 eventCycle = ULONG_MAX;
	
	if (itStoringChipsSnapshots != chipSnapshotsByCycle.end())
	{
		CStoredChipsSnapshot *chipsSnapshot = itStoringChipsSnapshots->second;
		chipsCycle = chipsSnapshot->cycle;
	}
	
	if (itStoringInputEvents != inputEventsByCycle.end())
	{
		CStoredInputEvent *inputEvent = itStoringInputEvents->second;
		eventCycle = inputEvent->cycle;
	}
	
	while (true)
	{
		// store chips till previous event cycle
		while(true)
		{
			if (itStoringChipsSnapshots != chipSnapshotsByCycle.end())
			{
				CStoredChipsSnapshot *chipsSnapshot = itStoringChipsSnapshots->second;
				chipsCycle = chipsSnapshot->cycle;
				
				if (chipsCycle >= maxCycle)
					break;
				
				if (chipsCycle <= eventCycle)
				{
					LOGD(".. store chipsSnapshot %d", chipsSnapshot->cycle);
					chipsSnapshot->StoreToFile(file);
					
					itStoringChipsSnapshots++;
					progressCurrentSnapshot++;
					
					progressStoreOrRestore = (float)progressCurrentSnapshot / (float)progressNumSnapshots;
					LOGD("///////////////////////// progressStoreOrRestore=%3.5f", progressStoreOrRestore);
				}
				else
				{
					// store events till this chips cycle
					break;
				}
			}
			else
			{
				break;
			}
		}
		
		// store events till next chips cycle
		while (true)
		{
			if (itStoringInputEvents != inputEventsByCycle.end())
			{
				CStoredInputEvent *inputEvent = itStoringInputEvents->second;
				eventCycle = inputEvent->cycle;
				
				if (eventCycle >= maxCycle)
					break;
				
				if (eventCycle < chipsCycle)
				{
					LOGD(".. store input event %d", inputEvent->cycle);
					inputEvent->StoreToFile(file);
					
					itStoringInputEvents++;
				}
			}
			else
			{
				break;
			}
		}
		
		bool chipsFinished = false;
		if (itStoringChipsSnapshots == chipSnapshotsByCycle.end()
			|| chipsCycle >= maxCycle)
		{
			chipsFinished = true;
		}
		
		bool eventsFinished = false;
		if (itStoringInputEvents == inputEventsByCycle.end()
			|| eventCycle >= maxCycle)
		{
			eventsFinished = true;
		}
		
		// stored all chips/events
		if (chipsFinished && eventsFinished)
			break;
	}
	
	LOGD("StoreChipsSnapshotsAndInputEventsTillCycle DONE cycle=%d", maxCycle);
}

bool CSnapshotsManager::RestoreTimelineSnapshotsFromFile(CSlrFile *file)
{
	// clear timeline
	this->ClearSnapshotsHistory();

	while (true)
	{
		if (file->Eof())
		{
			LOGError("CSnapshotsManager::RestoreTimelineSnapshotsFromFile: unexpected end of stream");
			return false;
		}
		
		u8 snapshotType = file->ReadU8();
		if (snapshotType == SNAPSHOT_TYPE_NONE)
		{
			// end of snapshots, all has been read correctly
			return true;
		}
		
		if (snapshotType == SNAPSHOT_TYPE_DISK)
		{
			CStoredDiskSnapshot *diskSnapshot = this->GetNewDiskSnapshot(file);
			diskSnapshotsByFrame[diskSnapshot->frame] = diskSnapshot;
			diskSnapshotsByCycle[diskSnapshot->cycle] = diskSnapshot;
		}
		else if (snapshotType == SNAPSHOT_TYPE_CHIPS)
		{
			CStoredChipsSnapshot *chipsSnapshot = this->GetNewChipSnapshot(file);
			chipSnapshotsByFrame[chipsSnapshot->frame] = chipsSnapshot;
			chipSnapshotsByCycle[chipsSnapshot->cycle] = chipsSnapshot;
			
			progressCurrentSnapshot++;
			progressStoreOrRestore = (float)progressCurrentSnapshot / (float)progressNumSnapshots;
			LOGD("///////////////////////// progressStoreOrRestore=%3.5f", progressStoreOrRestore);
		}
		else if (snapshotType == SNAPSHOT_TYPE_INPUT)
		{
			CStoredInputEvent *inputEvent = this->GetNewInputEventSnapshot(file);
			inputEventsByCycle[inputEvent->cycle] = inputEvent;
		}
	}
	
	return false;
}

void CSnapshotsManager::DebugPrintDiskSnapshots()
{
	LOGD(" ====== CSnapshotsManager::DebugPrintDiskSnapshots   DISKS ======");
	for (std::map<u32, CStoredDiskSnapshot *>::iterator it = diskSnapshotsByFrame.begin(); it != diskSnapshotsByFrame.end(); it++)
	{
		CStoredDiskSnapshot *diskSnapshot = it->second;
		
		LOGD("    | frame=%d ref=%d %x", diskSnapshot->frame, diskSnapshot->numLinkedChipsSnapshots, diskSnapshot);
	}
	LOGD("  diskSnapshotsToReuse=%d", diskSnapshotsToReuse.size());
	LOGD(" ===============================");

}

void CSnapshotsManager::DebugPrintChipsSnapshots()
{
	LOGD(" ====== CSnapshotsManager::DebugPrintChipsSnapshots   CHIPS ======");
	for (std::map<u32, CStoredChipsSnapshot *>::iterator it = chipSnapshotsByFrame.begin(); it != chipSnapshotsByFrame.end(); it++)
	{
		CStoredChipsSnapshot *chipsSnapshot = it->second;
		
		LOGD("    | frame=%d %x  disk frame=%d", chipsSnapshot->frame, chipsSnapshot, chipsSnapshot->diskSnapshot->frame);
	}
	LOGD(" ===============================");
}

void CSnapshotsManager::DebugPrintInputEventsSnapshots()
{
	LOGD(" ====== CSnapshotsManager::DebugPrintInputEventsSnapshots   ======");
	for (std::map<u64, CStoredInputEvent *>::iterator it = inputEventsByCycle.begin(); it != inputEventsByCycle.end(); it++)
	{
		CStoredInputEvent *inputEvent = it->second;
		
		LOGD("    | frame=%d %x  cycle=%d", inputEvent->frame, inputEvent, inputEvent->cycle);
	}
	LOGD(" ===============================");
}

void CSnapshotsManager::LockMutex()
{
//	LOGD("CSnapshotsManager::LockMutex");
	debugInterface->LockMutex();
//	mutex->Lock();
//	LOGD("CSnapshotsManager::LockMutex locked");
}

void CSnapshotsManager::UnlockMutex()
{
//	LOGD("CSnapshotsManager::UnlockMutex");
	debugInterface->UnlockMutex();
//	mutex->Unlock();
//	LOGD("CSnapshotsManager::UnlockMutex unlocked");
}
