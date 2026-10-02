#pragma once

#include <tge/non_copyable.hpp>
#include <tge/threading/timer_id.hpp>
#include <tge/threading/watch_id.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Tge::Threading
{
struct SEventLoopState;

// One thread that waits on descriptors, timers and posted work, and runs their callbacks one at a time. Callbacks
// must stay short: heavy work belongs on the job system.
class CEventLoop final : private SNoCopyNoMove
{
public:

	CEventLoop();
	~CEventLoop();

	bool Initialize(std::string_view threadName);
	// Joins once the pass in progress ends; whatever has not run by then is destroyed, never run.
	void Terminate();

	// Any thread, while the loop runs.
	void Post(std::function<void()> callback);

	// The rest is for the loop thread only, so set watches and timers up from a posted callback.
	// Level-triggered, and hang-up or error count as readable: a callback that reads end-of-file must Unwatch.
	std::optional<SWatchId> Watch(int descriptor, std::function<void()> onReadable);
	void Unwatch(SWatchId watch);
	STimerId ScheduleAt(std::chrono::steady_clock::time_point when, std::function<void()> callback);
	// Cancelling a timer that has already run does nothing.
	void CancelTimer(STimerId timer);

	bool IsLoopThread() const;

private:

	void Run(std::string threadName);
	void RunReadyWatches();
	void RunPosts();
	void RunDueTimers();
	std::optional<std::chrono::milliseconds> GetTimeUntilNextTimer() const;

	std::unique_ptr<SEventLoopState> m_state;
};
} // namespace Tge::Threading
