#pragma once
#include <mutex>
#include <atomic>
// Verified engine polarity and recursion, with depth instrumentation only.
// Depth is inspected on the owning thread or after a worker has joined.
class CSlrMutex {
 std::recursive_mutex mutex;
 std::atomic<int> depth{0};
public:
 bool TryLock() { if (!mutex.try_lock()) return false; ++depth; return true; }
 void Lock() { mutex.lock(); ++depth; }
 void Unlock() { --depth; mutex.unlock(); }
 int Depth() const { return depth.load(); }
};
