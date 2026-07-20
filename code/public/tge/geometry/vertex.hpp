#pragma once

#include <tge/math/types.hpp>
#include <cstdint>

namespace Tge::Geometry
{
struct SVertex final
{
	Math::Vec3 position;
	Math::Vec4 color;
	Math::Vec2 texCoord0;
	Math::Vec2 texCoord1;
	Math::Vec3 normal;
	Math::Vec3 tangent;
	Math::Vec3 bitangent;
};

// Held apart from SVertex so a shared vertex buffer is not widened for the non-skinned majority.
// std430 layout, uploaded verbatim; joints index the owning skin's palette, not any node array.
struct SSkinVertex final
{
	uint32_t   joints[4]{ 0u, 0u, 0u, 0u };
	Math::Vec4 weights{ 0.0f, 0.0f, 0.0f, 0.0f };
};

// Laid out [vertexLocal * numMorphTargets + target]. vec4 with w unused keeps the std430 array stride
// identical on CPU and GPU, sidestepping vec3 alignment padding.
struct SMorphDelta final
{
	Math::Vec4 position{ 0.0f, 0.0f, 0.0f, 0.0f };
	Math::Vec4 normal{ 0.0f, 0.0f, 0.0f, 0.0f };
	Math::Vec4 tangent{ 0.0f, 0.0f, 0.0f, 0.0f };
};
} // namespace Tge::Geometry
