#pragma once

#include <tge/threading/watch_id.hpp>
#include <functional>

namespace Tge::Threading
{
struct SEventLoopWatch final
{
	SWatchId id{};
	int descriptor{ -1 };
	std::function<void()> onReadable{};
};
} // namespace Tge::Threading
