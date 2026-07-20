#pragma once

#include <cstdint>

namespace Tge::Material
{
// glTF 2.0 material.alphaMode; the spec defines the semantics of each value.
enum class EAlphaMode : uint8_t
{
	Opaque,
	Mask,
	Blend
};
} // namespace Tge::Material
