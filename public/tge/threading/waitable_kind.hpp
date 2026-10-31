#pragma once

#include <cstdint>

namespace Tge::Threading
{
enum class EWaitableKind : uint8_t
{
	Socket,
	Event
};
} // namespace Tge::Threading
