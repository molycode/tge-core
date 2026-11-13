#pragma once

#include <tge/math/types.hpp>

#include <cstddef>
#include <cstdint>

namespace Tge::UiDraw
{
// Position in display pixels, colour as R, G, B, A bytes from the lowest address up -- ImGui's IM_COL32 order.
struct SUiVertex final
{
	Math::Vec2 position{};
	Math::Vec2 texCoord{};
	uint32_t   color{ 0xFFFFFFFFu };
};

// Pinned because a renderer uploads these verbatim and reads them by offset.
static_assert(sizeof(SUiVertex) == 20 && alignof(SUiVertex) == 4);
static_assert(offsetof(SUiVertex, position) == 0);
static_assert(offsetof(SUiVertex, texCoord) == 8);
static_assert(offsetof(SUiVertex, color) == 16);
} // namespace Tge::UiDraw
