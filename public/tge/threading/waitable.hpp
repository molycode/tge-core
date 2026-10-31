#pragma once

#include <tge/threading/waitable_kind.hpp>
#include <cstdint>
#include <limits>

namespace Tge::Threading
{
struct SWaitable final
{
	uintptr_t native{ std::numeric_limits<uintptr_t>::max() };
	EWaitableKind kind{ EWaitableKind::Socket };
};
} // namespace Tge::Threading
