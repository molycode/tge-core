#pragma once

#include <filesystem>

namespace Tge::Process
{
bool IsRunnable(std::filesystem::path const& path);
} // namespace Tge::Process
