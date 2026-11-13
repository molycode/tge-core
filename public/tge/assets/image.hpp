#pragma once

#include <cstdint>
#include <vector>

namespace Tge::Assets
{
// Top-down rows of 8-bit RGBA, as decoded: what a UI uploads, without a model's mips, sampler or usage.
struct SImage final
{
	uint32_t             width{ 0 };
	uint32_t             height{ 0 };
	std::vector<uint8_t> pixels;
};

enum class EImageError : uint8_t
{
	NotFound,
	Unreadable,
	Undecodable,
};
} // namespace Tge::Assets
