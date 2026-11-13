#pragma once

#include <cstdint>
#include <limits>

namespace Tge::UiDraw
{
// Issued by whatever draws the UI; a producer only passes back what it was given.
using UiTextureHandle = uint32_t;

inline constexpr UiTextureHandle InvalidUiTextureHandle{ std::numeric_limits<UiTextureHandle>::max() };
} // namespace Tge::UiDraw
