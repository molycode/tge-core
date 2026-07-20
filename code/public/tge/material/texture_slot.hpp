#pragma once

#include <cstdint>

namespace Tge::Material
{
// Names a material's texture-transform slots. The declaration ORDER is a contract: a renderer may index
// its GPU material array by this enum's value, so inserting a slot mid-list silently rebinds the others.
enum class ETextureSlot : uint8_t
{
	BaseColor,
	Normal,
	Emissive,
	MetallicRoughness,
	Occlusion,
	SpecularColor,
	Anisotropy,
	Transmission,
	Iridescence,
	IridescenceThickness,
	Clearcoat,
	ClearcoatRoughness,
	SheenColor,
	SheenRoughness,
	ClearcoatNormal,
	Thickness,
	Specular,
	DiffuseTransmission,
	DiffuseTransmissionColor
};
} // namespace Tge::Material
