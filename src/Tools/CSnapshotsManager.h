#ifndef _CSNAPSHOTSMANAGER_H_
#define _CSNAPSHOTSMANAGER_H_

#include "CDebugInterface.h"
#include "CGuiViewProgressBarWindow.h"
#include "CTimelineNavigation.h"
#include "CTimelineSeekTransaction.h"

#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <atomic>

#define RDTL_MAGIC		0x5244544C
#define RDTL_VERSION	0x0001

#define SNAPSHOT_TYPE_NONE	0
#define SNAPSHOT_TYPE_CHIPS	1
#define SNAPSHOT_TYPE_DISK	2
#define SNAPSHOT_TYPE_INPUT	3

class CSnapshotsManager;
class CViewTimeline;
class CSlrFile;

class CStoredSnapshot
{
public:
	CStoredSnapshot(CSnapshotsManager *manager, u8 snapshotType);
	virtual ~CStoredSnapshot();
	
	CSnapshotsManager *manager;
		
	virtual void Use(u32 frame, u64 cycle);
	virtual void Clear();
	
	CByteBuffer *byteBuffer;
	
	u32 frame;
	u64 cycle;
	
	u8 snapshotType;
	// Transient capture receipt; not serialized into legacy timeline files.
	// A true backend return is not a promise about unsupported external media.
	bool captureSucceeded = false;
	virtual void StoreToFile(CSlrFile *file);
	virtual void RestoreFromFile(CSlrFile *file);
};

class CStoredDiskSnapshot : public CStoredSnapshot
{
public:
	CStoredDiskSnapshot(CSnapshotsManager *manager, u32 frame, u64 cycle);
	CStoredDiskSnapshot(CSnapshotsManager *manager, CSlrFile *file);
	
	int numLinkedChipsSnapshots;
	
	void AddReference();
	void RemoveReference();
	
	virtual void StoreToFile(CSlrFile *file);
	virtual void RestoreFromFile(CSlrFile *file);
};

class CStoredChipsSnapshot : public CStoredSnapshot
{
public:
	CStoredChipsSnapshot(CSnapshotsManager *manager, u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot);
	CStoredChipsSnapshot(CSnapshotsManager *manager, CSlrFile *file);

	virtual void Use(u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot);
	
	CStoredDiskSnapshot *diskSnapshot;

	virtual void Clear();
	
	virtual void StoreToFile(CSlrFile *file);
	virtual void RestoreFromFile(CSlrFile *file);
};

// Note: we tread stored input event as snapshot, as we can store multiple events at the same cycle
class CStoredInputEvent : public CStoredSnapshot
{
public:
	CStoredInputEvent(CSnapshotsManager *manager, u32 frame, u64 cycle);
	CStoredInputEvent(CSnapshotsManager *manager, CSlrFile *file);

//	virtual void StoreToFile(CSlrFile *file);
//	virtual void RestoreFromFile(CSlrFile *file);
};

class CSnapshotsManager : public CGuiViewProgressBarWindowCallback
{
public:
	CSnapshotsManager(CDebugInterface *debugInterface);
	virtual ~CSnapshotsManager();
	
	CDebugInterface *debugInterface;
	CViewTimeline *viewTimeline;
	
	std::map<u32, CStoredChipsSnapshot *> chipSnapshotsByFrame;
	std::map<u64, CStoredChipsSnapshot *> chipSnapshotsByCycle;
	// TODO: add and use lower_bound when searching for cycle: std::map<u64, CStoredChipsSnapshot *> chipSnapshotsByCycle;
	std::list<CStoredChipsSnapshot *> chipsSnapshotsToReuse;
	
	std::map<u32, CStoredDiskSnapshot *> diskSnapshotsByFrame;
	std::map<u64, CStoredDiskSnapshot *> diskSnapshotsByCycle;
	std::list<CStoredDiskSnapshot *> diskSnapshotsToReuse;
	
	// replay input events
	std::map<u64, CStoredInputEvent *> inputEventsByCycle;
	std::list<CStoredInputEvent *> inputEventsToReuse;
	CStoredInputEvent *nextInputEvent;
	u64 lastReplayedCycle;  // tracks which events have been replayed (non-destructive)

	virtual bool CheckSnapshotInterval();
	
	CStoredDiskSnapshot *currentDiskSnapshot;
	
	CStoredChipsSnapshot *snapshotToRestore;
	virtual bool CheckSnapshotRestore();
	
	virtual void RestoreSnapshot(CStoredChipsSnapshot *snapshot);
	virtual bool RestoreSnapshotByFrame(int frame);
	virtual bool RestoreSnapshotByFrame(int frame, std::int64_t cycleNum);
	virtual bool RestoreSnapshotByFrame(int frame, std::int64_t cycleNum, u8 targetDebugMode);
	virtual bool RestoreSnapshotByCycle(u64 cycle);
	virtual bool RestoreSnapshotByCycle(u64 cycle, u8 targetDebugMode);

	// On-demand coarse navigation only; accepted does not mean restored.
	TimelineNavigation::Status GetFrameNavigationStatus();
	TimelineNavigation::Result RequestFrameNavigation(int frame);

private:
	TimelineNavigation::Status FrameNavigationStatusLocked();
public:
	
	// realtime recording snapshots, get new or reuse empty snapshots from pool
	CStoredDiskSnapshot *GetNewDiskSnapshot(u32 frame, u64 cycle);
	CStoredChipsSnapshot *GetNewChipSnapshot(u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot);
	CStoredInputEvent *GetNewInputEventSnapshot(u32 frame, u64 cycle);
	
	// reading snapshots from file, get new or reuse empty snapshots from pool, read from file
	CStoredDiskSnapshot *GetNewDiskSnapshot(CSlrFile *file);
	CStoredChipsSnapshot *GetNewChipSnapshot(CSlrFile *file);
	CStoredInputEvent *GetNewInputEventSnapshot(CSlrFile *file);

	bool CheckMainCpuCycle();

	// Main CPU only, at pre-dispatch and immediately before opcode execution.
	// This is a finite contiguous boundary index, not per-address markers.
	void TimelineInstructionBoundary(bool executable = true, u64 clocksPerSecond = 0);
	void TimelineInstructionDispatch(bool executable = true, u64 clocksPerSecond = 0);
	bool GetTimelineInstructionCoverage(u64 &first, u64 &last, std::size_t &count, bool &atBoundary);

	TimelineSeek::Status GetTimelineStatus();
	TimelineSeek::Result PerformTimelineSeek(const TimelineSeek::Request &request, u32 timeoutMs);
	// CPU-thread pause observation, after native register import and refresh.
	// A queued request alone must never unwind a paused instruction.
	void TimelinePausePoint(const TimelineSeek::Position &position);
	bool TimelineCanPublishPause();
	void TimelinePauseRegisters(u16 pc, u8 a, u8 x, u8 y, u8 p, u8 sp);
	void TimelineAfterRegisterImport();
	bool TimelineNeedsCpuService() const { return timelineTransaction.NeedsCpuService(); }
	bool IsStrictTimelineSeekActive() const { return timelineActive.load(std::memory_order_acquire); }
	bool IsStrictTimelineImportPending() const { return IsStrictTimelineSeekActive() && timelineImportPending.load(std::memory_order_acquire); }
	
	void ResetLastStoredFrameCounter();
	void ClearSnapshotsHistory();
	void DeleteAllPools();
	
	void RestoreSnapshotByNumFramesOffset(int numFramesOffset);
	void RestoreSnapshotBackstepInstruction();
	
	int pauseNumFrame;
	// -1 means no cycle target; long is only 32 bits on Windows.
	std::int64_t pauseNumCycle;
	volatile bool skipFrameRender;
	bool SkipRefreshOfVideoFrame();
	
	volatile bool isPerformingSnapshotRestore;
	bool IsPerformingSnapshotRestore();
	void CancelRestore();
	
	//
	// should we store input events?
	bool isStoreInputEventsEnabled;
	
	// should we replay input events at current cycle?
	bool isReplayInputEventsEnabled;
	
	CByteBuffer *StoreNewInputEventsSnapshotAtCurrentCycle();
	bool CheckInputEventsAtCurrentCycle();
	void TruncateInputEventsAfterCycle(u64 cycle);

	bool SaveInputEventsToFile(const char *filePath);
	bool LoadInputEventsFromFile(const char *filePath);
	
	//
//	volatile bool skipSavingSnapshots;
	
	void SetRecordingIsActive(bool isActive);
	void SetRecordingStoreInterval(int recordingInterval);
	void SetRecordingLimit(int recordingLimit);
	
	void GetFramesLimits(int *minFrame, int *maxFrame);

	static int progressNumSnapshots;
	static int progressCurrentSnapshot;
	static float progressStoreOrRestore;	// for progress bar
	virtual float GetGuiViewProgressBarWindowValue(void *userData);

	void StoreTimelineToFile(CSlrString *filePath);
	void StoreTimelineSnapshotsToFile(CSlrFile *file);
	void StoreChipsSnapshotsAndInputEventsTillCycle(CSlrFile *file, u64 cycle);
	static bool RestoreTimelineFromFile(CSlrString *filePath);
	bool RestoreTimelineSnapshotsFromFile(CSlrFile *file);
	
	std::map<u64, CStoredDiskSnapshot *>::iterator itStoringDiskSnapshots;
	std::map<u64, CStoredChipsSnapshot *>::iterator itStoringChipsSnapshots;
	std::map<u64, CStoredInputEvent *>::iterator itStoringInputEvents;

	//
	void DebugPrintDiskSnapshots();
	void DebugPrintChipsSnapshots();
	void DebugPrintInputEventsSnapshots();

	//
	void LockMutex();
	void UnlockMutex();

	// External (off-emulation-thread) chips-snapshot request — used by the
	// remote/MCP server and tests via CDebugInterface::*AtCpuBoundary.
	// Executed by the emulation thread inside CheckSnapshotRestore() at the
	// next CPU instruction boundary (polled every ~16ms while paused);
	// blocks the caller until completed there or timeoutMs elapsed.
	// The paused-mode guarantee (a parked CPU unwinds to consume the request
	// instead of timing out) applies to the C64/VICE pause architecture via
	// IsExternalSnapshotRequestActive(); Atari/NES paused-mode requests may
	// time out (running mode works on all three).
	// Must not be called from the emulation thread (it would self-deadlock
	// until the timeout).
	static const int EXTERNAL_SNAPSHOT_REQUEST_NONE = 0;
	static const int EXTERNAL_SNAPSHOT_REQUEST_LOAD = 1;
	static const int EXTERNAL_SNAPSHOT_REQUEST_SAVE = 2;
	bool PerformExternalSnapshotRequest(int requestType, CByteBuffer *buffer, u32 timeoutMs);

	// True while an external snapshot request is pending or being executed.
	// Lock-free racy read — emulation-side pause loops use it to unwind to the
	// next CPU instruction boundary instead of parking. No side effects
	// (unlike IsPerformingSnapshotRestore).
	bool IsExternalSnapshotRequestActive() { return externalSnapshotRequestType != EXTERNAL_SNAPSHOT_REQUEST_NONE || externalSnapshotInProgress; }

	// True while a snapshot is actually being written or read, no matter who
	// asked for it.
	//
	// Writing a snapshot runs the drive CPUs forward to an instruction
	// boundary (c64_snapshot_write_in_memory -> drive_cpu_execute_all), and
	// those CPUs call c64d_debug_pause_check(0), which parks while the
	// debugger is PAUSED. If the caller doing the snapshot is the same thread
	// that would have to unpause, it waits for itself and the app hangs --
	// observed as CTestViceSnapshot stalling for 10+ minutes.
	//
	// IsExternalSnapshotRequestActive() already gave that unwind guarantee,
	// but only to callers going through PerformExternalSnapshotRequest().
	// Calling Save/LoadChipsSnapshotSynced() directly -- which the tests, and
	// anything holding a CDebugInterface, legitimately do -- got no
	// protection at all. This flag attaches it to the operation instead of to
	// one entry point. Counted, because ConsumeExternalSnapshotRequestUnlocks
	// wraps a call that raises it again.
	bool IsSnapshotOperationInProgress() { return snapshotOperationDepth > 0; }

	// RAII: raise the flag for the duration of one snapshot save/load.
	class CSnapshotOperationScope
	{
	public:
		CSnapshotOperationScope(CSnapshotsManager *manager) : manager(manager)
		{
			if (manager) manager->snapshotOperationDepth++;
		}
		~CSnapshotOperationScope()
		{
			if (manager && manager->snapshotOperationDepth > 0) manager->snapshotOperationDepth--;
		}
	private:
		CSnapshotsManager *manager;
	};

private:
	// Lock order: manager mutex -> timeline history mutex.  The CPU producer
	// never calls a manager operation while holding the history mutex.
	// Coarse request: manager -> admission -> transaction.  Precise Submit
	// holds admission -> transaction only, never manager; release before Wait.
	// CPU intake/publication retain manager -> transaction.  No reverse edge.
	std::mutex timelineAdmissionMutex;
	TimelineSeek::Transaction timelineTransaction;
	std::atomic<bool> timelineActive{false};
	// Native fields below are protected by the manager mutex.  CPU observations
	// and admission never retain pointers owned by the remote caller.
	TimelineSeek::Result timelineNativeResult;
	CStoredChipsSnapshot *timelineCheckpoint = nullptr;
	u64 timelineEpoch = 0, timelineAdmittedEpoch = 0;
	std::atomic<bool> timelineImportPending{false};
	bool timelineLoaded = false;
	bool timelineTargetReached = false, timelineRequireBoundary = false;
	u64 timelineTargetCounter = 0;
	std::string timelineFailure;
	TimelineSeek::Bounds TimelineBoundsLocked(const TimelineSeek::Position *origin);
	void TimelineStopLocked(const char *reason);
	void TimelineCheckCancellation();

	std::mutex timelineHistoryMutex;
	TimelineSeek::InstructionHistory timelineInstructionHistory;
	// Main-CPU rate observations use the same history lock.  A rate transition
	// bounds time conversion; never substitute nominal display FPS.
	u64 timelineClockRate = 0, timelineTimebaseStart = 0;
	std::atomic<bool> timelineAtInstructionBoundary{false};
	// CPU-thread-owned cache avoids a second history lock on ordinary dispatch.
	u64 timelineLastBoundary = 0;
	bool timelineBoundaryExecutable = false;

	CSlrMutex *mutex;
	u32 lastStoredFrame;
	u32 lastStoredFrameCounter;

	// pending external request; guarded by the manager mutex (LockMutex)
	volatile int externalSnapshotRequestType;
	CByteBuffer *externalSnapshotBuffer;
	// true while the request is being executed by ConsumeExternalSnapshotRequestUnlocks
	// (i.e. after it has been picked off externalSnapshotRequestType but before
	// completion is signaled) — lock-free racy read via IsExternalSnapshotRequestActive()
	volatile bool externalSnapshotInProgress;
	// Depth of nested snapshot save/load operations; see
	// IsSnapshotOperationInProgress().
	volatile int snapshotOperationDepth;
	// completion handshake between requester thread and emulation thread
	std::mutex externalSnapshotRequestSerializeMutex;
	std::mutex externalSnapshotSignalMutex;
	std::condition_variable externalSnapshotSignalCV;
	bool externalSnapshotCompleted;
	bool externalSnapshotResult;
	// generation numbers guard against a stale completion signal (from an
	// operation that only finished after PerformExternalSnapshotRequest's 30s
	// fallback wait gave up) poisoning a later, unrelated request
	u64 externalSnapshotRequestSeq;
	u64 externalSnapshotPendingSeq;
	u64 externalSnapshotCompletedSeq;
	// Consumes the pending request on the emulation thread. Called by
	// CheckSnapshotRestore() with the manager mutex HELD; UNLOCKS it before
	// returning. Returns true when a LOAD was performed — the CPU loop must
	// then re-import registers (same contract as a timeline restore).
	bool ConsumeExternalSnapshotRequestUnlocks();
};

#endif //_CSNAPSHOTSMANAGER_H_

