#pragma once

#include <tge/math/types.hpp>
#include <tge/ui_draw/texture_handle.hpp>

#include <cstdint>

namespace Tge::UiDraw
{
// Clip rectangle as (min x, min y, max x, max y) in display pixels.
struct SUiDrawCommand final
{
	Math::Vec4      clipRect{};
	UiTextureHandle texture{ InvalidUiTextureHandle };
	uint32_t        numIndices{ 0u };
	uint32_t        firstIndex{ 0u };
	uint32_t        vertexOffset{ 0u };
};

static_assert(sizeof(SUiDrawCommand) == 32 && alignof(SUiDrawCommand) == 4);
} // namespace Tge::UiDraw
