#pragma once

#include <tge/io/user_folder.hpp>
#include <expected>
#include <filesystem>
#include <system_error>

namespace Tge::IO
{
std::expected<std::filesystem::path, std::error_code> GetUserFolder(EUserFolder folder);
} // namespace Tge::IO
