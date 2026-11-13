#pragma once

#include <tge/ui_draw/draw_command.hpp>
#include <tge/ui_draw/vertex.hpp>

#include <cstdint>
#include <span>

namespace Tge::UiDraw
{
struct SUiDrawList final
{
	std::span<SUiVertex const>      vertices{};
	std::span<uint32_t const>       indices{};
	std::span<SUiDrawCommand const> commands{};
};
} // namespace Tge::UiDraw
