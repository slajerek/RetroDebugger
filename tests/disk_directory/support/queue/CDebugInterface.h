#pragma once
#include "../backend_mutex.h"
#include "CDebugInterfaceTask.h"
#include <list>
#include <atomic>
#include <new>
// Only the container is instrumented; AddCpuDebugInterruptTask is extracted
// verbatim from product source by prepare_controls.py, not copied here.
class CDebugInterface {
 CSlrMutex mutex;
public:
 CSlrMutex *tasksMutex = &mutex;
 struct Queue {
  CSlrMutex *mutex;
  std::list<CDebugInterfaceTask *> entries;
  bool failNext = false;
  int depthAtFailure = 0;
  void push_back(CDebugInterfaceTask *task) {
   if (failNext) {
    failNext = false;
    depthAtFailure = mutex->Depth();
    throw std::bad_alloc();
   }
   entries.push_back(task);
  }
 } cpuDebugInterruptTasks{&mutex};
 std::atomic<bool> hasPendingCpuDebugInterruptTasks{false};
 void LockTasksMutex() { tasksMutex->Lock(); }
 void UnlockTasksMutex() { tasksMutex->Unlock(); }
 void AddCpuDebugInterruptTask(CDebugInterfaceTask *task);
};
