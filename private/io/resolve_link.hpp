#pragma once

#include <filesystem>

namespace Tge::IO
{
std::filesystem::path ResolveLink(std::filesystem::path const& path);
} // namespace Tge::IO
