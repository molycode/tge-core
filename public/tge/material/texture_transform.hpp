#pragma once

#include <tge/math/types.hpp>
#include <cstdint>

namespace Tge::Material
{
// glTF KHR_texture_transform.
struct STextureTransform final
{
	Math::Vec2 scale{ Math::Vec2One };
	Math::Vec2 offset{ Math::Vec2Zero };
	float      rotation{ 0.0f };  // radians, counter-clockwise
	uint32_t   texCoord{ 0 };     // UV set: 0 = TEXCOORD_0, 1 = TEXCOORD_1

	bool operator==(STextureTransform const&) const = default;

	bool IsIdentity() const
	{
		return scale.x == 1.0f && scale.y == 1.0f &&
		       offset.x == 0.0f && offset.y == 0.0f &&
		       rotation == 0.0f && texCoord == 0;
	}
};

static_assert(sizeof(STextureTransform) == 24 && alignof(STextureTransform) == 4);
} // namespace Tge::Material
