#pragma once

#include <tge/math/types.hpp>
#include <cstdint>

namespace Tge::Probe
{
// A probe adjustment volume's intrinsics, without placement: its owner's transform places, turns and scales it,
// as a node places a light.
struct SAdjustmentVolume final
{
	// Consumers mirror these values as bare literals, so they are ABI.
	enum class EMode : uint32_t { Exclude = 0, ForceActive = 1 };
	enum class EShape : uint32_t { Box = 0, Sphere = 1 };

	EMode      mode{ EMode::Exclude };
	EShape     shape{ EShape::Box };
	Math::Vec3 halfExtents{ 0.5f };  // box only (m)
	float      radius{ 0.5f };       // sphere only (m)
};

static_assert(sizeof(SAdjustmentVolume) == 24 && alignof(SAdjustmentVolume) == 4);

static_assert(static_cast<uint32_t>(SAdjustmentVolume::EMode::Exclude)     == 0u);
static_assert(static_cast<uint32_t>(SAdjustmentVolume::EMode::ForceActive) == 1u);
static_assert(static_cast<uint32_t>(SAdjustmentVolume::EShape::Box)        == 0u);
static_assert(static_cast<uint32_t>(SAdjustmentVolume::EShape::Sphere)     == 1u);
} // namespace Tge::Probe
