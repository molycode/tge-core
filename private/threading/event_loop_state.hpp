#pragma once

#include "event_loop_timer.hpp"
#include "event_loop_watch.hpp"
#include "poller.hpp"
#include <tge/threading/mpsc_queue.hpp>
#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

namespace Tge::Threading
{
struct SEventLoopState final
{
	CPoller poller{};
	std::thread thread{};
	std::atomic<bool> isStopping{ false };
	CMpscQueue<std::function<void()>> posts{};
	std::vector<std::function<void()>> runningPosts{};
	std::vector<SEventLoopWatch> watches{};
	std::vector<SEventLoopTimer> timers{};
	std::vector<uint64_t> readyKeys{};
	uint64_t nextId{ 0 };
};
} // namespace Tge::Threading
