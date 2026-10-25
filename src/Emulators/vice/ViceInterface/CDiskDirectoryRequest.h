#ifndef _CDISKDIRECTORYREQUEST_H_
#define _CDISKDIRECTORYREQUEST_H_

#include "CDebugInterface.h"
#include "CDebugInterfaceTask.h"
#include "SYS_Threading.h"
#include "json.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

// Feature-local only.  CSlrMutex::TryLock returns true on acquisition (both
// SDL3 and std::recursive_mutex implementations); nested backend locks recurse.
class CDiskDirectoryTryLock
{
	CSlrMutex *mutex;
public:
	const bool locked;
	explicit CDiskDirectoryTryLock(CSlrMutex *mutex) : mutex(mutex), locked(mutex && mutex->TryLock()) {}
	~CDiskDirectoryTryLock() { if (locked) mutex->Unlock(); }
	CDiskDirectoryTryLock(const CDiskDirectoryTryLock &) = delete;
	CDiskDirectoryTryLock &operator=(const CDiskDirectoryTryLock &) = delete;
};

struct CDiskDirectoryCompletion
{
	using Clock = std::chrono::steady_clock;
	enum class Phase { Pending, Executing, Completed, Expired };
	std::mutex mutex;
	std::condition_variable changed;
	Phase phase = Phase::Pending;
	const Clock::time_point deadline;
	nlohmann::json result;

	explicit CDiskDirectoryCompletion(Clock::time_point deadline) : deadline(deadline) {}

	bool Begin()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (phase != Phase::Pending) return false;
		if (Clock::now() >= deadline)
		{
			phase = Phase::Expired;
			changed.notify_all();
			return false;
		}
		phase = Phase::Executing;
		return true;
	}

	void Complete(nlohmann::json value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (phase == Phase::Executing)
		{
			if (Clock::now() >= deadline) phase = Phase::Expired;
			else
			{
				result = std::move(value);
				phase = Phase::Completed;
			}
		}
		changed.notify_all();
	}

	nlohmann::json Wait()
	{
		std::unique_lock<std::mutex> lock(mutex);
		changed.wait_until(lock, deadline, [this] {
			return phase == Phase::Completed || phase == Phase::Expired;
		});
		if (phase == Phase::Completed) return std::move(result);
		phase = Phase::Expired;
		return {{"status", "timeout"}, {"reason", "cpu_task_deadline"}};
	}
};

class CDiskDirectoryTask : public CDebugInterfaceTask
{
	std::shared_ptr<std::atomic<bool>> slot;
	std::shared_ptr<CDiskDirectoryCompletion> completion;
	std::function<nlohmann::json()> read;
	bool admitted = false;
public:
	CDiskDirectoryTask(std::shared_ptr<std::atomic<bool>> slot,
		std::shared_ptr<CDiskDirectoryCompletion> completion, std::function<nlohmann::json()> read)
	: slot(std::move(slot)), completion(std::move(completion)), read(std::move(read)) {}

	bool Admit()
	{
		bool empty = false;
		admitted = slot->compare_exchange_strong(empty, true);
		return admitted;
	}
	~CDiskDirectoryTask() override
	{
		// Only this task can release its slot, including after requester expiry.
		// No endpoint object, VICE pointer, or backend lock is touched at retirement.
		if (admitted) slot->store(false);
	}
	void ExecuteTask() override
	{
		if (!completion->Begin()) return;
		// No completion lock around backend work.  Late data is discarded.
		try { completion->Complete(read()); }
		catch (...) { completion->Complete({{"status", "unavailable"}, {"reason", "backend_exception"}}); }
	}
};

inline nlohmann::json RequestDiskDirectory(CDebugInterface *debugInterface,
	const std::shared_ptr<std::atomic<bool>> &slot, std::function<nlohmann::json()> read)
{
	auto completion = std::make_shared<CDiskDirectoryCompletion>(
		CDiskDirectoryCompletion::Clock::now() + std::chrono::milliseconds(1000));
	std::unique_ptr<CDiskDirectoryTask> task(new CDiskDirectoryTask(slot, completion, std::move(read)));
	if (!task->Admit()) return {{"status", "busy"}, {"reason", "outstanding_request"}};
	{
		CDiskDirectoryTryLock queueLock(debugInterface->tasksMutex);
		if (!queueLock.locked) return {{"status", "busy"}, {"reason", "task_queue_locked"}};
		// Add recurses into tasksMutex.  The queue alone executes and deletes it.
		try { debugInterface->AddCpuDebugInterruptTask(task.get()); }
		catch (...) { return {{"status", "unavailable"}, {"reason", "task_enqueue_failed"}}; }
		task.release();
	}
	return completion->Wait();
}

#endif
