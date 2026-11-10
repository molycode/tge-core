#pragma once

#include <string>

namespace Tge::Assets
{
// Content roots, written by the composition root before the runtime initializes the provider. Every root that
// can hold GUID-bearing assets is indexed at startup, so a GUID minted last session resolves this one.
struct SSettings final
{
	std::string modelPath{ "assets/models/" };
	std::string scenePath{ "assets/scenes/" };
	std::string testPath{ "assets/tests/" };
	std::string hdriPath{ "assets/hdri/" };
};

extern SSettings gSettings;
} // namespace Tge::Assets
