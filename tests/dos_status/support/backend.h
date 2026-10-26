#pragma once
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <stdexcept>
#include <functional>
#ifdef DOS_STATUS_TEST_REAL_ENQUEUE
#include <list>
#include <memory>
#endif

// Deliberate small doubles.  They do not validate native VICE or SDL behavior.
class CSlrMutex
{
	std::recursive_mutex mutex;
	std::atomic<unsigned int> depth{0};
public:
	bool TryLock() { if (!mutex.try_lock()) return false; ++depth; return true; }
	void Lock() { mutex.lock(); ++depth; }
	void Unlock() { --depth; mutex.unlock(); }
	unsigned int Depth() const { return depth.load(); }
};
class CDebugInterfaceTask
{
public:
	inline static std::atomic<unsigned int> destroyed{0};
	virtual ~CDebugInterfaceTask() { ++destroyed; }
	virtual void ExecuteTask() = 0;
};
#ifdef DOS_STATUS_TEST_REAL_ENQUEUE
// Throw from the actual list allocation, after the production method's inner lock.
inline std::function<void()> beforeTaskAllocation;
template<class T> struct TaskAllocator : std::allocator<T>
{
	template<class U> struct rebind { using other = TaskAllocator<U>; };
	T *allocate(std::size_t count)
	{
		if (beforeTaskAllocation) beforeTaskAllocation();
		return std::allocator<T>::allocate(count);
	}
};
#endif
class CDebugInterfaceVice
{
	CSlrMutex queue, interface, io;
public:
	CSlrMutex *tasksMutex = &queue, *breakpointsMutex = &interface, *ioMutex = &io;
#ifdef DOS_STATUS_TEST_REAL_ENQUEUE
	std::list<CDebugInterfaceTask *, TaskAllocator<CDebugInterfaceTask *>> cpuDebugInterruptTasks;
	decltype(cpuDebugInterruptTasks) &tasks = cpuDebugInterruptTasks;
	std::atomic<bool> hasPendingCpuDebugInterruptTasks{false};
	void LockTasksMutex() { tasksMutex->Lock(); }
	void UnlockTasksMutex() { tasksMutex->Unlock(); }
	void AddCpuDebugInterruptTask(CDebugInterfaceTask *task);
#else
	std::deque<CDebugInterfaceTask *> tasks;
#endif
	std::mutex noticeMutex;
	std::condition_variable notice;
	std::atomic<unsigned int> adds{0};
	bool paused = true;
#ifndef DOS_STATUS_TEST_REAL_ENQUEUE
	void AddCpuDebugInterruptTask(CDebugInterfaceTask *task)
	{
		tasksMutex->Lock();
		tasks.push_back(task);
		{ std::lock_guard<std::mutex> lock(noticeMutex); ++adds; }
		notice.notify_all();
		tasksMutex->Unlock();
	}
#endif
	void AwaitAdd(unsigned int count)
	{
		std::unique_lock<std::mutex> lock(noticeMutex);
		if (!notice.wait_for(lock, std::chrono::seconds(2), [&] { return adds >= count; }))
			throw std::runtime_error("task not admitted");
	}
	void Drain()
	{
		tasksMutex->Lock();
		while (!tasks.empty())
		{
			auto *task = tasks.front(); tasks.pop_front();
			task->ExecuteTask(); delete task;
		}
		tasksMutex->Unlock();
	}
};

extern "C" {
enum { DRIVE_TYPE_1541 = 1541, DRIVE_TYPE_1541II = 1542, DRIVE_IDLE_TRAP_IDLE = 2, BUFFER_COMMAND_CHANNEL = 5, CBMDOS_FAM_READ = 0 };
struct drive_t { unsigned int type = 1541; void *image = nullptr; };
struct drive_context_t
{
	drive_t *drive = nullptr;
	unsigned int enable = 1, type = 1541;
	int profdos = 0, supercard = 0, stardos = 0;
	void *ds1216 = nullptr;
	int drive_ram2_enabled = 0, drive_ram4_enabled = 0, drive_ram6_enabled = 0;
	int drive_ram8_enabled = 0, drive_rama_enabled = 0, idling_method = 0;
	uint8_t rom[32768]{}, trap_rom[32768]{}, drive_ram[2048]{};
};
struct bufferinfo_t
{
	unsigned int mode = 5, readmode = 0, bufptr = 0, length = 0;
	uint8_t *buffer = nullptr;
};
struct vdrive_t { bufferinfo_t buffers[16]; int last_code = 0; void *image = nullptr; };
extern drive_context_t *drive_context[4];
int resources_get_int(const char *, int *);
vdrive_t *file_system_get_vdrive(unsigned int);
}
