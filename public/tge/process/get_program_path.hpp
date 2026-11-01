#pragma once

#include <expected>
#include <filesystem>
#include <system_error>

namespace Tge::Process
{
std::expected<std::filesystem::path, std::error_code> GetProgramPath();
} // namespace Tge::Process
