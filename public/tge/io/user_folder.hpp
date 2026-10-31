#pragma once

#include <cstdint>

namespace Tge::IO
{
enum class EUserFolder : uint8_t
{
	Config,
	Data,
	State,
	Cache
};
} // namespace Tge::IO
