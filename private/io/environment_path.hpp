#pragma once

#include <filesystem>

namespace Tge::IO
{
// Empty when the variable is unset.
std::filesystem::path GetEnvironmentPath(char const* pName);
} // namespace Tge::IO
