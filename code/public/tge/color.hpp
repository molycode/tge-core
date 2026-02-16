#pragma once

#include <cstdint>

namespace Tge
{
struct SColor final
{
	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 255;
	uint8_t a = 255;

	constexpr SColor() = default;
	explicit constexpr SColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
		: r(red), g(green), b(blue), a(alpha)
	{
	}
};

struct SColorHDR final
{
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
	float a = 1.0f;

	constexpr SColorHDR() = default;
	explicit constexpr SColorHDR(float red, float green, float blue, float alpha = 1.0f)
		: r(red), g(green), b(blue), a(alpha)
	{
	}
};
} // namespace Tge
