#pragma once

#include <cstdint>
#include <limits>

namespace Tge::Threading
{
struct SWatchId final
{
	uint64_t value{ std::numeric_limits<uint64_t>::max() };
};
} // namespace Tge::Threading
