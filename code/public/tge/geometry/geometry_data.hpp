#pragma once

#include <tge/geometry/vertex.hpp>
#include <vector>
#include <variant>
#include <cstdint>

namespace Tge::Geometry
{
struct SGeometryData final
{
	std::vector<SVertex> vertices;
	std::variant<std::vector<uint16_t>, std::vector<uint32_t>> indices;
};
} // namespace Tge::Geometry
