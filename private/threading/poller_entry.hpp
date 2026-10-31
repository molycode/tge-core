#pragma once

#include <tge/threading/waitable.hpp>
#include <cstdint>
#include <limits>

namespace Tge::Threading
{
struct SPollerEntry final
{
	SWaitable waitable{};
	uint64_t key{ std::numeric_limits<uint64_t>::max() };
};
} // namespace Tge::Threading
