#pragma once

// Feature-local, portable DOS-status parsing and request lifetime rules.
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace DosStatus
{
struct Snapshot
{
	std::string outcome = "unavailable";
	std::string reason;
	std::string backend;
	std::string firmwareSha256;
	std::string text;
	std::vector<uint8_t> raw;
	unsigned int driveType = 0;
	unsigned int pointer = 0, end = 0, mode = 0, readMode = 0, channelState = 0;
	int lastCode = -1, code = -1, track = -1, sector = -1;
	bool mediaPresent = false, mediaKnown = false;
};

inline Snapshot Refuse(Snapshot s, const char *outcome, const char *reason)
{
	s.outcome = outcome;
	s.reason = reason;
	return s;
}

// A bounded syntax interpretation, NOT a claim about who last wrote drive RAM.
inline Snapshot ParseText(Snapshot s, size_t length)
{
	if (length < 11 || length > s.raw.size())
		return Refuse(s, "transient", "incomplete buffered status");
	std::string text(s.raw.begin(), s.raw.begin() + length);
	for (unsigned char c : text)
		if (c < 32 || c > 126)
			return Refuse(s, "unsupported", "binary or non-text buffered reply");
	auto digit = [](char c) { return c >= '0' && c <= '9'; };
	size_t separator = text.find(',', 3);
	if (!digit(text[0]) || !digit(text[1]) || text[2] != ','
		|| separator == std::string::npos || separator == 3 || separator + 6 != length
		|| !digit(text[separator + 1]) || !digit(text[separator + 2])
		|| text[separator + 3] != ',' || !digit(text[separator + 4]) || !digit(text[separator + 5]))
		return Refuse(s, "transient", "malformed or incomplete buffered status");
	s.code = (text[0] - '0') * 10 + text[1] - '0';
	s.track = (text[separator + 1] - '0') * 10 + text[separator + 2] - '0';
	s.sector = (text[separator + 4] - '0') * 10 + text[separator + 5] - '0';
	s.text = text;
	s.outcome = "complete";
	s.reason.clear();
	return s;
}

inline Snapshot ParseStock1541(Snapshot s)
{
	// 325302-01 + 901229-05: E6C7 sets A5/A6 to 02D5 while building;
	// E6FC stores the LAST sector digit's low address in 0249; E6FF
	// advances A5 to 02D6.  There is no CR in the workspace itself.
	if (s.raw.size() != 36 || s.end < 0xD5 || s.end > 0xF8
		|| s.pointer < 0x2D6 || s.pointer > 0x200 + s.end || s.channelState != 0x88)
		return Refuse(s, "transient", "workspace pointer/end/channel state is not a supported complete buffer");
	return ParseText(s, s.end - 0xD5 + 1);
}

inline Snapshot ParseVirtual(Snapshot s, unsigned int commandMode, unsigned int readMode)
{
	if (s.mode != commandMode || s.readMode != readMode)
		return Refuse(s, "unsupported", "command channel is not a readable status reply");
	if (s.lastCode == 4)
		return Refuse(s, "unsupported", "virtual memory-read reply is not DOS status");
	if (s.end >= s.raw.size() || s.pointer > s.end)
		return Refuse(s, "transient", "invalid virtual reply bounds");
	if (s.raw[s.end] != 13)
		return Refuse(s, "unsupported", "virtual reply has no status terminator");
	s = ParseText(s, s.end);
	if (s.outcome == "complete" && s.code != s.lastCode)
	{
		s.text.clear();
		s.code = s.track = s.sector = -1;
		return Refuse(s, "unsupported", "virtual reply does not match last_code");
	}
	return s;
}

using Clock = std::chrono::steady_clock;
enum class Phase { pending, executing, completed, expired };
struct Request
{
	explicit Request(Clock::time_point deadline) : deadline(deadline) {}
	std::mutex mutex;
	std::condition_variable changed;
	const Clock::time_point deadline;
	Phase phase = Phase::pending;
	Snapshot result;

	bool Start()
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (phase != Phase::pending || Clock::now() >= deadline)
		{
			phase = Phase::expired;
			changed.notify_all();
			return false;
		}
		phase = Phase::executing;
		return true;
	}
	void Finish(Snapshot value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (phase == Phase::executing && Clock::now() < deadline)
		{
			result = std::move(value);
			phase = Phase::completed;
		}
		else
			phase = Phase::expired;
		changed.notify_all();
	}
	Snapshot Wait()
	{
		std::unique_lock<std::mutex> lock(mutex);
		changed.wait_until(lock, deadline, [this] { return phase == Phase::completed || phase == Phase::expired; });
		if (phase == Phase::completed)
			return result;
		phase = Phase::expired;
		return Refuse(Snapshot(), "timeout", "CPU task deadline expired; slot remains busy until retirement");
	}
};

// This slot belongs to the feature, not an endpoint or requester lifetime.
class Slot
{
	std::mutex mutex;
	std::shared_ptr<Request> active;
public:
	std::shared_ptr<Request> Reserve(Clock::time_point deadline)
	{
		std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
		if (!lock.owns_lock() || active)
			return {};
		active = std::make_shared<Request>(deadline);
		return active;
	}
	void Retire(const std::shared_ptr<Request> &request)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (active == request)
			active.reset();
	}
};
}
