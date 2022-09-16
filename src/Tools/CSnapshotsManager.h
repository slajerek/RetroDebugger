#ifndef _CSNAPSHOTSMANAGER_H_
#define _CSNAPSHOTSMANAGER_H_

#include "CDebugInterface.h"
#include "CGuiViewProgressBarWindow.h"

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

	virtual bool CheckSnapshotInterval();
	
	CStoredDiskSnapshot *currentDiskSnapshot;
	
	CStoredChipsSnapshot *snapshotToRestore;
	virtual bool CheckSnapshotRestore();
	
	virtual void RestoreSnapshot(CStoredChipsSnapshot *snapshot);
	virtual bool RestoreSnapshotByFrame(int frame, long cycleNum);
	virtual bool RestoreSnapshotByFrame(int frame, long cycleNum, u8 targetDebugMode);
	virtual bool RestoreSnapshotByCycle(u64 cycle);
	virtual bool RestoreSnapshotByCycle(u64 cycle, u8 targetDebugMode);
	
	// realtime recording snapshots, get new or reuse empty snapshots from pool
	CStoredDiskSnapshot *GetNewDiskSnapshot(u32 frame, u64 cycle);
	CStoredChipsSnapshot *GetNewChipSnapshot(u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot);
	CStoredInputEvent *GetNewInputEventSnapshot(u32 frame, u64 cycle);
	
	// reading snapshots from file, get new or reuse empty snapshots from pool, read from file
	CStoredDiskSnapshot *GetNewDiskSnapshot(CSlrFile *file);
	CStoredChipsSnapshot *GetNewChipSnapshot(CSlrFile *file);
	CStoredInputEvent *GetNewInputEventSnapshot(CSlrFile *file);

	bool CheckMainCpuCycle();
	
	void ResetLastStoredFrameCounter();
	void ClearSnapshotsHistory();
	void DeleteAllPools();
	
	void RestoreSnapshotByNumFramesOffset(int numFramesOffset);
	void RestoreSnapshotBackstepInstruction();
	
	int pauseNumFrame;
	long pauseNumCycle;
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
	
	// should we clear (overwrite) input events at current cycle?
	bool isOverwriteInputEventsEnabled;
	
	CByteBuffer *StoreNewInputEventsSnapshotAtCurrentCycle();
	bool CheckInputEventsAtCurrentCycle();
	
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
	
private:
	CSlrMutex *mutex;
	u32 lastStoredFrame;
	u32 lastStoredFrameCounter;
};

#endif //_CSNAPSHOTSMANAGER_H_

