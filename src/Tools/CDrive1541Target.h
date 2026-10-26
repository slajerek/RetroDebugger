#ifndef _CDRIVE1541TARGET_H_
#define _CDRIVE1541TARGET_H_

#include <atomic>
#include <cstdint>
#include <mutex>

// One feature-owned target.  No breakpoint-container or request/segment pointers.
class CDrive1541Target
{
public:
	struct Observation
	{
		uint64_t requestId;
		uint16_t pc;
		bool active;
		bool hit;
	};

	CDrive1541Target() : activeOwner(0), target(0), requestId(0), hit(false) {}

	// Caller holds the new-control admission guard and has observed a pause.
	uint64_t Arm(uint16_t pc)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (++requestId == 0) ++requestId;
		target.store(pc, std::memory_order_relaxed);
		hit = false;
		activeOwner.store(requestId, std::memory_order_release);
		return requestId;
	}

	// Called only at drive0's pre-instruction boundary.  Inactive cost: one load.
	// The pause action and cancellation serialize, so cancellation cannot return
	// while this target still has a pending feature-owned pause action.
	template<class Pause>
	bool AtBoundary(uint16_t pc, Pause pause)
	{
		const uint64_t owner = activeOwner.load(std::memory_order_acquire);
		if (owner == 0) return false;
		if (target.load(std::memory_order_relaxed) != pc) return false;
		std::lock_guard<std::mutex> lock(mutex);
		if (activeOwner.load(std::memory_order_relaxed) != owner || target.load(std::memory_order_relaxed) != pc) return false;
		if (!pause()) return false; // Caller may reject a concurrent mode change.
		hit = true;
		activeOwner.store(0, std::memory_order_release);
		return true;
	}

	Observation Observe()
	{
		std::lock_guard<std::mutex> lock(mutex);
		return Current();
	}

	Observation Cancel(uint64_t owner)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (owner == requestId) activeOwner.store(0, std::memory_order_release);
		return Current();
	}

private:
	Observation Current() const
	{
		return {requestId, target.load(std::memory_order_relaxed), activeOwner.load(std::memory_order_relaxed) != 0, hit};
	}
	std::mutex mutex;
	std::atomic<uint64_t> activeOwner;
	std::atomic<uint16_t> target;
	uint64_t requestId;
	bool hit;
};

#endif
