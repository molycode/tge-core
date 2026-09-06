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
	// Everything from Tube on has EXTENT, and a consumer that does not know a value shades it as a point --
	// which is wrong but not broken, and is what the three older values already meant.
	enum class EType : uint32_t { Directional = 0, Point = 1, Spot = 2, Tube = 3, Rect = 4, Disc = 5 };

	EType      type{ EType::Point };
	Math::Vec3 color{ Math::Vec3One };
	float      intensity{ 1.0f };
	float      range{ 0.0f };                      // 0 = infinite
	float      innerConeAngle{ 0.0f };             // half-angle in radians (spot only)
	float      outerConeAngle{ Math::QuarterPi };  // half-angle in radians (spot only, per spec)
	float      sourceRadius{ 0.35f };              // the emitter's own size: geometry within it does not occlude this light

	// The emitter's extent about its primary axis, which every consumer pairs with the placement it holds
	// separately. A disc needs none of these -- it is sourceRadius about the axis -- and that is the point of
	// keeping the radius where it already is rather than folding it in here.
	float      halfWidth{ 0.0f };                  // tube half-length, or rect half-extent along the tangent (m)
	float      halfHeight{ 0.0f };                 // rect half-extent across the tangent (m)
	float      roll{ 0.0f };                       // the tangent's rotation about the primary axis (radians)
	float      coneAspect{ 1.0f };                 // spot only: the cone's across/along half-angle ratio; 1 = round
};

static_assert(sizeof(SLight) == 52 && alignof(SLight) == 4);

static_assert(static_cast<uint32_t>(SLight::EType::Directional) == 0u);
static_assert(static_cast<uint32_t>(SLight::EType::Point)       == 1u);
static_assert(static_cast<uint32_t>(SLight::EType::Spot)        == 2u);
static_assert(static_cast<uint32_t>(SLight::EType::Tube)        == 3u);
static_assert(static_cast<uint32_t>(SLight::EType::Rect)        == 4u);
static_assert(static_cast<uint32_t>(SLight::EType::Disc)        == 5u);

// True for every type whose emitter has extent, i.e. the ones a point-light approximation gets wrong.
constexpr bool HasArea(SLight::EType type)
{
	return type == SLight::EType::Tube || type == SLight::EType::Rect || type == SLight::EType::Disc;
}
} // namespace Tge::Light
