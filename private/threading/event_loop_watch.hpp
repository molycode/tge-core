#pragma once

#include <tge/threading/waitable.hpp>
#include <tge/threading/watch_id.hpp>
#include <functional>

namespace Tge::Threading
{
struct SEventLoopWatch final
{
	SWatchId id{};
	SWaitable waitable{};
	std::function<void()> onReadable{};
};
} // namespace Tge::Threading
