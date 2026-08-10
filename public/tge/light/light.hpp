#pragma once

#include <tge/math/constants.hpp>
#include <tge/math/types.hpp>
#include <cstdint>

namespace Tge::Light
{
// A light's intrinsics, without placement: in glTF the position is the owning node's, and a renderer's copy
// of it is a cache of that transform. Each consumer pairs this with placement in whatever form suits it.
struct SLight final
{
	// Consumers mirror these values as bare literals with nothing cross-checking them, so they are ABI.
	enum class EType : uint32_t { Directional = 0, Point = 1, Spot = 2 };

	EType      type{ EType::Point };
	Math::Vec3 color{ Math::Vec3One };
	float      intensity{ 1.0f };
	float      range{ 0.0f };                      // 0 = infinite
	float      innerConeAngle{ 0.0f };             // half-angle in radians (spot only)
	float      outerConeAngle{ Math::QuarterPi };  // half-angle in radians (spot only, per spec)
};

static_assert(sizeof(SLight) == 32 && alignof(SLight) == 4);

static_assert(static_cast<uint32_t>(SLight::EType::Directional) == 0u);
static_assert(static_cast<uint32_t>(SLight::EType::Point)       == 1u);
static_assert(static_cast<uint32_t>(SLight::EType::Spot)        == 2u);
} // namespace Tge::Light
