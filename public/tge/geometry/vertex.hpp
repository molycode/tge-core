#pragma once

#include <tge/math/types.hpp>
#include <cstddef>
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

// Offsets are pinned because these upload verbatim and are read back by offset, so a reorder corrupts
// without changing the size.
static_assert(sizeof(SVertex)  == 80 && alignof(SVertex)  == 4);
static_assert(offsetof(SVertex, position)  ==  0);
static_assert(offsetof(SVertex, color)     == 12);
static_assert(offsetof(SVertex, texCoord0) == 28);
static_assert(offsetof(SVertex, texCoord1) == 36);
static_assert(offsetof(SVertex, normal)    == 44);
static_assert(offsetof(SVertex, tangent)   == 56);
static_assert(offsetof(SVertex, bitangent) == 68);

static_assert(sizeof(SSkinVertex) == 32 && alignof(SSkinVertex) == 4);
static_assert(offsetof(SSkinVertex, joints)  ==  0);
static_assert(offsetof(SSkinVertex, weights) == 16);

static_assert(sizeof(SMorphDelta) == 48 && alignof(SMorphDelta) == 4);
static_assert(offsetof(SMorphDelta, position) ==  0);
static_assert(offsetof(SMorphDelta, normal)   == 16);
static_assert(offsetof(SMorphDelta, tangent)  == 32);
} // namespace Tge::Geometry
