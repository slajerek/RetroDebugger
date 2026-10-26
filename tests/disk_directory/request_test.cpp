#include "CDiskDirectoryRequest.h"
#include "CDiskDirectoryProtocol.h"
#include <future>
#include <iostream>
#include <stdexcept>
using json = nlohmann::json;
using Clock = CDiskDirectoryCompletion::Clock;
using Phase = CDiskDirectoryCompletion::Phase;
static int checks;
static void check(bool ok, const char *message) {
 ++checks; if (!ok) throw std::runtime_error(message);
}
int main() {
 try {
  unsigned char name[] = {'A',0,0xff,0xa0,0x20};
  auto rendered=DirectoryName(name,sizeof(name));
  name[0]='B';
  check(rendered["display"]=="A??" && rendered["petscii"]==json::array({65,0,255,160,32}),
        "lossy display with exact independently owned PETSCII");
  int device;
  for (int d=8; d<=11; ++d) check(ParseDiskDirectoryDevice({{"device",d}},device) && device==d,"device");
  check(ParseDiskDirectoryDevice(json::object(),device) && device==8,"default");
  for (const json &v : {json(7),json(12),json(-1),json(8.0),json("8"),json(true),json(nullptr),json(UINT64_MAX)})
   check(!ParseDiskDirectoryDevice({{"device",v}},device),"invalid device rejected");
  check(!ParseDiskDirectoryDevice({{"platform","c64u"}},device),"no Ultimate routing");
  check(!ParseDiskDirectoryDevice(json::array(),device),"object required");
  auto slot=std::make_shared<std::atomic<bool>>(false);
  int calls=0;
  auto expired=std::make_shared<CDiskDirectoryCompletion>(Clock::now()-std::chrono::seconds(1));
  {
   CDiskDirectoryTask task(slot,expired,[&] { ++calls; return json(); });
   check(task.Admit(),"admit"); task.ExecuteTask();
   check(calls==0 && expired->Wait()["status"]=="timeout" && *slot,"expired skips backend, holds slot");
   { CDiskDirectoryTask other(slot,expired,[] { return json(); });
     check(!other.Admit(),"one task limit"); }
   check(*slot,"rejected task retirement cannot release incumbent");
  }
  check(!*slot,"retirement releases slot");
  auto done=std::make_shared<CDiskDirectoryCompletion>(Clock::now()+std::chrono::seconds(2));
  {
   CDiskDirectoryTask task(slot,done,[&] {
    check(done->mutex.try_lock(),"completion mutex not held across backend");
    done->mutex.unlock(); ++calls; return json{{"status","ok"},{"owned",json::array({1,2})}};
   });
   check(task.Admit(),"admit success"); task.ExecuteTask(); task.ExecuteTask();
   check(done->Wait()["owned"][1]==2 && calls==1 && *slot,"owned reply, execute once, occupied until retirement");
  }
  auto active=std::make_shared<CDiskDirectoryCompletion>(Clock::now()+std::chrono::milliseconds(100));
  std::promise<void> entered, finish;
  auto finishFuture=finish.get_future();
  CSlrMutex media;
  auto task=std::make_unique<CDiskDirectoryTask>(slot,active,[&] {
   CDiskDirectoryTryLock lock(&media);
   if (!lock.locked) throw std::runtime_error("test media lock");
   entered.set_value(); finishFuture.wait(); return json{{"status","late"}};
  });
  check(task->Admit(),"active admit");
  auto worker=std::async(std::launch::async,[&] { task->ExecuteTask(); });
  entered.get_future().wait();
  check(active->Wait()["status"]=="timeout" && *slot,"executing timeout does not reclaim");
  finish.set_value(); worker.get();
  check(active->phase==Phase::Expired && active->result.is_null(),"late reply discarded");
  task.reset(); check(!*slot,"late retirement");

  CDebugInterface di;
  std::promise<void> locked, release;
  auto releaseFuture=release.get_future();
  auto holder=std::async(std::launch::async,[&] {
   di.tasksMutex->Lock(); locked.set_value(); releaseFuture.wait(); di.tasksMutex->Unlock();
  });
  locked.get_future().wait();
  { CDiskDirectoryTryLock lock(di.tasksMutex); check(!lock.locked,"backend try-lock contention"); }
  auto begin=Clock::now();
  auto busy=RequestDiskDirectory(&di,slot,[] { return json(); });
  check(busy["reason"]=="task_queue_locked" && Clock::now()-begin<std::chrono::milliseconds(500),
        "admission never blocks");
  check(!*slot && di.queue.empty(),"failed admission retains nothing");
  release.set_value(); holder.get();
  { CDiskDirectoryTryLock a(di.tasksMutex), b(di.tasksMutex);
    check(a.locked && b.locked,"recursive acquisition polarity"); }

  // No queue servicing: bounded wait leaves one owned callback for later retirement.
  auto reply=RequestDiskDirectory(&di,slot,[&] { ++calls; return json(); });
  check(reply["status"]=="timeout" && *slot && di.queue.size()==1,"unserviced pause timeout");
  for (int i=0; i<20; ++i)
   check(RequestDiskDirectory(&di,slot,[] { return json(); })["status"]=="busy" && di.queue.size()==1,
         "repeated retries cannot grow retained queue");
  di.Drain(); check(!*slot && calls==1,"expired queue callback does no backend work");

  std::promise<void> queued;
  auto queuedFuture=queued.get_future();
  di.onQueued=[&] { queued.set_value(); };
  auto service=std::async(std::launch::async,[&] { queuedFuture.wait(); di.Drain(); });
  check(RequestDiskDirectory(&di,slot,[] { return json{{"status","ok"}}; })["status"]=="ok",
        "queue released before normal request wait");
  service.get(); check(!*slot && di.queue.empty(),"queue retires successful task exactly once");
  di.onQueued={};
  auto failed=std::make_shared<CDiskDirectoryCompletion>(Clock::now()+std::chrono::seconds(2));
  { CDiskDirectoryTask bad(slot,failed,[]() -> json { throw std::runtime_error("test"); });
    check(bad.Admit(),"exception admission"); bad.ExecuteTask();
    check(failed->Wait()["reason"]=="backend_exception","backend exception is not success"); }
  check(!*slot,"exception retirement");
  std::cout << "PASS request/protocol: " << checks << " checks\n";
 } catch (const std::exception &e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
