#ifndef _CSNAPSHOTSMANAGER_H_
#define _CSNAPSHOTSMANAGER_H_

#include "CDebugInterface.h"

class CSnapshotsManager;

class CStoredSnapshot
{
public:
	CStoredSnapshot(CSnapshotsManager *manager);
	virtual ~CStoredSnapshot();
	
	CSnapshotsManager *manager;
	
	virtual void Use(u32 frame, u64 cycle);
	virtual void Clear();
	
	CByteBuffer *byteBuffer;
	
	u32 frame;
	u64 cycle;
};

class CStoredDiskSnapshot : public CStoredSnapshot
{
public:
	CStoredDiskSnapshot(CSnapshotsManager *manager, u32 frame, u64 cycle);
	
	int numLinkedChipsSnapshots;
	
	void AddReference();
	void RemoveReference();
};

class CStoredChipsSnapshot : public CStoredSnapshot
{
public:
	CStoredChipsSnapshot(CSnapshotsManager *manager, u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot);
	
	virtual void Use(u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot);
	
	CStoredDiskSnapshot *diskSnapshot;

	virtual void Clear();
};

// Note: we tread stored input event as snapshot, as we can store multiple events at the same cycle
class CStoredInputEvent : public CStoredSnapshot
{
public:
	CStoredInputEvent(CSnapshotsManager *manager, u32 frame, u64 cycle);
};

class CSnapshotsManager
{
public:
	CSnapshotsManager(CDebugInterface *debugInterface);
	~CSnapshotsManager();
	
	CDebugInterface *debugInterface;
	
	std::map<u32, CStoredChipsSnapshot *> chipSnapshotsByFrame;
	// TODO: add and use lower_bound when searching for cycle: std::map<u64, CStoredChipsSnapshot *> chipSnapshotsByCycle;
	std::list<CStoredChipsSnapshot *> chipsSnapshotsToReuse;
	
	std::map<u32, CStoredDiskSnapshot *> diskSnapshotsByFrame;
	// std::map<u64, CStoredChipsSnapshot *> diskSnapshotsByCycle;
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
	virtual bool RestoreSnapshotByCycle(u64 cycle);
	
	CStoredChipsSnapshot *GetNewChipSnapshot(u32 frame, u64 cycle, CStoredDiskSnapshot *diskSnapshot);
	CStoredDiskSnapshot *GetNewDiskSnapshot(u32 frame, u64 cycle);
	CStoredInputEvent *GetNewInputEventSnapshot(u32 frame, u64 cycle);
	
	bool CheckMainCpuCycle();
	
	void ResetLastStoredFrameCounter();
	void ClearSnapshotsHistory();
	
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

//	void StoreToFile(CSlrString *filePath);
//	void RestoreFromFile(CSlrString *filePath);

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

