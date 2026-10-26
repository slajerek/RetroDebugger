#pragma once
#include <mutex>
// Mirror verified engine polarity and recursive semantics, not native scheduling.
class CSlrMutex {
 std::recursive_mutex mutex;
public:
 bool TryLock() { return mutex.try_lock(); }
 void Lock() { mutex.lock(); }
 void Unlock() { mutex.unlock(); }
};
