#pragma once

#include <tge/math/types.hpp>
#include <tge/ui_draw/draw_list.hpp>

#include <span>

namespace Tge::UiDraw
{
// One frame's UI as a producer hands it over: views into memory the producer keeps until it is consumed.
struct SUiDrawData final
{
	std::span<SUiDrawList const> lists{};

	Math::Vec2 displayPosition{ 0.0f, 0.0f };
	Math::Vec2 displaySize{ 0.0f, 0.0f };
};
} // namespace Tge::UiDraw
