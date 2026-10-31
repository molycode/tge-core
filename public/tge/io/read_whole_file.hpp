#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <system_error>

namespace Tge::IO
{
std::expected<std::string, std::error_code> ReadWholeFile(std::filesystem::path const& path, size_t maxSize);
} // namespace Tge::IO
