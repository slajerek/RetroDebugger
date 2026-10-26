#include "CDiskDirectoryRequest.h"
#include "queue.inc"
#include <future>
#include <iostream>
using json = nlohmann::json;
int main() {
 CDebugInterface di;
 auto slot = std::make_shared<std::atomic<bool>>(false);
 auto token = std::make_shared<int>(42);
 std::weak_ptr<int> lifetime = token;
 auto read = [owned = std::move(token)] { return json{{"status", "ok"}}; };
 di.cpuDebugInterruptTasks.failNext = true;
 json result;
 bool threw = false;
 try { result = RequestDiskDirectory(&di, slot, std::move(read)); }
 catch (const std::bad_alloc &) { threw = true; }
 const bool unavailable = !threw && result.value("status", "") == "unavailable";
 const bool destroyed = lifetime.expired();
 const bool released = !*slot;
 const bool insertedNothing = di.cpuDebugInterruptTasks.entries.empty()
     && !di.hasPendingCpuDebugInterruptTasks;
 const bool foreign = std::async(std::launch::async, [&] {
  bool acquired = di.tasksMutex->TryLock();
  if (acquired) di.tasksMutex->Unlock();
  return acquired;
 }).get();
 bool reuse = false;
 {
  auto completion = std::make_shared<CDiskDirectoryCompletion>(CDiskDirectoryCompletion::Clock::now());
  CDiskDirectoryTask next(slot, completion, [] { return json(); });
  reuse = next.Admit();
 }
 const bool inner = di.cpuDebugInterruptTasks.depthAtFailure == 2;
 std::cout << "inner_depth=" << di.cpuDebugInterruptTasks.depthAtFailure
           << " unavailable=" << unavailable << " threw=" << threw
           << " task_destroyed=" << destroyed << " slot_released=" << released
           << " slot_reused=" << reuse << " foreign_reacquired=" << foreign
           << " queue_unchanged=" << insertedNothing << "\n";
 // Clean up only AFTER recording the failed foreign-thread control.  Avoid
 // destroying a locked test mutex on the deliberately broken pre-fix run.
 while (di.tasksMutex->Depth()) di.tasksMutex->Unlock();
 bool ok = inner && unavailable && destroyed && released && reuse && foreign && insertedNothing && !*slot;
 std::cout << (ok ? "PASS" : "FAIL") << " enqueue exception control\n";
 return ok ? 0 : 1;
}
