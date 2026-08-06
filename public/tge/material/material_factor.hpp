#pragma once

#include <cstdint>

namespace Tge::Material
{
// The material factors KHR_animation_pointer channels may target; each maps to one SMaterialProperties
// field. alphaMode, pointSize and clearcoatNormalScale are deliberately absent — not spec-animatable, or
// animating them would force a per-frame material reclassification.
enum class EMaterialFactor : uint8_t
{
	BaseColor,
	Emissive,
	EmissiveStrength,
	Metallic,
	Roughness,
	AlphaCutoff,
	Specular,
	SpecularColor,
	Ior,
	Clearcoat,
	ClearcoatRoughness,
	SheenColor,
	SheenRoughness,
	Transmission,
	AnisotropyStrength,
	AnisotropyRotation,
	Thickness,
	AttenuationDistance,
	AttenuationColor,
	Dispersion,
	Iridescence,
	IridescenceIor,
	IridescenceThicknessMin,
	IridescenceThicknessMax
};
} // namespace Tge::Material
