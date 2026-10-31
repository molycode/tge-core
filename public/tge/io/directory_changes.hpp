#pragma once

#include <filesystem>
#include <vector>

namespace Tge::IO
{
struct SDirectoryChanges final
{
	std::vector<std::filesystem::path> paths{};
	bool hasOverflowed{ false };
};
} // namespace Tge::IO
