#pragma once

#include <tge/material/texture_slot.hpp>
#include <tge/material/material_factor.hpp>
#include <tge/index.hpp>
#include <string>
#include <vector>
#include <cstdint>

namespace Tge::Model
{
// glTF animation sampler interpolation modes (glTF 2.0 animation.sampler.interpolation).
enum class EInterpolation : uint8_t
{
	Linear,
	Step,
	CubicSpline
};

// What a parsed animation channel drives (see SAnimationChannel for the per-kind targetIndex meaning).
enum class EAnimationTarget : uint8_t
{
	NodeTranslation,
	NodeRotation,
	NodeScale,
	NodeVisibility,
	TexTransformOffset,
	TexTransformScale,
	TexTransformRotation,
	MaterialFactor,
	MorphWeight
};

// One animation sampler: keyframe times (input) paired with flattened output values.
// componentCount is the logical element width (1=scalar, 2=vec2, 3=vec3, 4=quat). CubicSpline
// output holds 3 * componentCount floats per keyframe (in-tangent, value, out-tangent);
// Linear/Step hold componentCount floats per keyframe.
struct SAnimationSampler final
{
	std::vector<float> times;
	std::vector<float> values;
	uint8_t            componentCount{ 0 };
	EInterpolation     interpolation{ EInterpolation::Linear };
};

// One animation channel binding a sampler to a typed target. targetIndex is a source glTF node
// index for Node* targets, or a source glTF material index for TexTransform* / MaterialFactor targets.
// textureSlot is meaningful only for TexTransform* targets; materialFactor only for MaterialFactor.
struct SAnimationChannel final
{
	uint32_t                       samplerIndex{ InvalidIndex };
	EAnimationTarget          target{ EAnimationTarget::NodeTranslation };
	uint32_t                       targetIndex{ InvalidIndex };
	Material::ETextureSlot    textureSlot{ Material::ETextureSlot::BaseColor };
	Material::EMaterialFactor materialFactor{ Material::EMaterialFactor::BaseColor };
};

// One named animation clip: its samplers + channels and total duration in seconds.
struct SAnimation final
{
	std::string                    name;
	std::vector<SAnimationSampler> samplers;
	std::vector<SAnimationChannel> channels;
	float                          duration{ 0.0f };
};
} // namespace Tge::Model
