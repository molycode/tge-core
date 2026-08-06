#pragma once

#include <cstdint>

namespace Tge::Geometry
{
enum class EPrimitiveTopology : uint8_t
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip
};
} // namespace Tge::Geometry
