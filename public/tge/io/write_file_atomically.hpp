#pragma once

#include <expected>
#include <filesystem>
#include <string_view>
#include <system_error>

namespace Tge::IO
{
std::expected<void, std::error_code> WriteFileAtomically(std::filesystem::path const& path, std::string_view content);
} // namespace Tge::IO
