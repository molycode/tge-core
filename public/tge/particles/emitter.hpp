#pragma once

#include <string>

namespace Tge::Particles
{
// A particle emitter's intrinsics, without placement: its owner's transform places and turns it, as a node
// places a light.
struct SEmitter final
{
	std::string effect;                 // the effect's name, resolved by whoever simulates it
	float       intensity{ 1.0f };      // scales the effect's spawn rates
	bool        active{ true };         // false stops spawning; live particles finish their lives
};

static_assert(sizeof(SEmitter) == sizeof(std::string) + 8 && alignof(SEmitter) == alignof(std::string));
} // namespace Tge::Particles
