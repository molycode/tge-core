#pragma once

#include <tge/material/alpha_mode.hpp>
#include <tge/math/types.hpp>
#include <tge/math/constants.hpp>
#include <cstdint>

namespace Tge::Material
{
// KHR_materials_ior default.
constexpr float IOR{ 1.5f };

// Engine default for POINTS topology; glTF has no equivalent.
constexpr float PointSize{ 5.0f };

struct SMaterialProperties final
{
	// PBR base (glTF 2.0 core spec)
	Math::Vec4 baseColorFactor{ Math::Vec4One };
	Math::Vec3 emissiveFactor{ Math::Vec3Zero };
	float metallicFactor{ 1.0f };
	float roughnessFactor{ 1.0f };

	// glTF core normalTextureInfo.scale (normal-map strength)
	float normalScale{ 1.0f };

	// glTF core occlusionTextureInfo.strength (baked-AO influence)
	float occlusionStrength{ 1.0f };

	// KHR_materials_specular extension
	float specularFactor{ 1.0f };
	Math::Vec3 specularColorFactor{ Math::Vec3One };

	EAlphaMode alphaMode{ EAlphaMode::Opaque };
	float alphaCutoff{ 0.5f };

	bool doubleSided{ false };

	// KHR_materials_unlit extension
	bool isUnlit{ false };

	// A baked flat normal cannot survive skinning (a face's three vertices can ride different joints), so a
	// primitive without NORMAL is shaded from the derived geometric normal instead — glTF 3.7.2.1.
	bool hasVertexNormals{ true };

	// Tge-specific: false where an analytic light already carries this emitter, so indirect must not count it twice.
	bool emissiveIndirect{ true };

	// KHR_materials_emissive_strength extension
	float emissiveStrength{ 1.0f };

	// KHR_materials_ior extension
	float ior{ IOR };

	// KHR_materials_transmission extension
	float transmissionFactor{ 0.0f };

	// KHR_materials_diffuse_transmission extension
	float diffuseTransmissionFactor{ 0.0f };
	Math::Vec3 diffuseTransmissionColorFactor{ Math::Vec3One };

	// KHR_materials_clearcoat extension
	float clearcoatFactor{ 0.0f };
	float clearcoatRoughnessFactor{ 0.0f };
	float clearcoatNormalScale{ 1.0f };

	// KHR_materials_sheen extension
	Math::Vec3 sheenColorFactor{ Math::Vec3Zero };
	float sheenRoughnessFactor{ 0.0f };

	// KHR_materials_anisotropy extension
	float anisotropyStrength{ 0.0f };
	float anisotropyRotation{ 0.0f };

	// KHR_materials_volume extension
	float thicknessFactor{ 0.0f };
	float attenuationDistance{ Math::Infinity };
	Math::Vec3 attenuationColor{ Math::Vec3One };

	// KHR_materials_dispersion extension
	float dispersion{ 0.0f };

	// KHR_materials_iridescence extension
	float iridescenceFactor{ 0.0f };
	float iridescenceIor{ 1.3f };
	float iridescenceThicknessMinimum{ 100.0f };  // nanometers
	float iridescenceThicknessMaximum{ 400.0f };  // nanometers

	float pointSize{ PointSize };  // pixels, POINTS topology only

	bool operator==(SMaterialProperties const&) const = default;
};

// Size only: at this field count per-offset pins would be noise, so a same-size reorder still passes here.
static_assert(sizeof(SMaterialProperties) == 180 && alignof(SMaterialProperties) == 4);
} // namespace Tge::Material
