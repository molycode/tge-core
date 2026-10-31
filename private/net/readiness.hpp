#pragma once

#include <cstdint>

namespace Tge::Net
{
enum class EReadiness : uint8_t
{
	Readable,
	Writable
};
} // namespace Tge::Net
