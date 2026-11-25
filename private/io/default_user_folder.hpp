#pragma once

#include <tge/io/user_folder.hpp>
#include <expected>
#include <filesystem>
#include <system_error>

namespace Tge::IO
{
// The folder the system names when nothing overrides it.
std::expected<std::filesystem::path, std::error_code> GetDefaultUserFolder(EUserFolder folder);
} // namespace Tge::IO
