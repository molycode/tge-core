#pragma once

#include <tge/threading/timer_id.hpp>
#include <chrono>
#include <functional>

namespace Tge::Threading
{
struct SEventLoopTimer final
{
	STimerId id{};
	std::chrono::steady_clock::time_point when{};
	std::function<void()> callback{};
};
} // namespace Tge::Threading
