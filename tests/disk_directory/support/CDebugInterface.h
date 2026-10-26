#pragma once
#include "SYS_Threading.h"
#include "CDebugInterfaceTask.h"
#include <vector>
#include <functional>
class CDebugInterface {
 CSlrMutex queueMutex;
public:
 CSlrMutex *tasksMutex = &queueMutex;
 std::vector<CDebugInterfaceTask *> queue;
 std::function<void()> onQueued;
 void AddCpuDebugInterruptTask(CDebugInterfaceTask *task) {
  tasksMutex->Lock(); queue.push_back(task); tasksMutex->Unlock();
  if (onQueued) onQueued();
 }
 void Drain() {
  tasksMutex->Lock();
  for (auto *task : queue) { task->ExecuteTask(); delete task; }
  queue.clear(); tasksMutex->Unlock();
 }
};
