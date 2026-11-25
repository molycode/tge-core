#pragma once

#include <tge/io/user_folder.hpp>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace Tge::IO::Tests
{
using Variables = std::vector<std::pair<char const*, std::optional<std::string>>>;

std::optional<std::string> GetVariable(char const* pName);
void SetVariable(char const* pName, std::optional<std::string> const& value);

// Asks with these variables set or unset, then puts each back as it was.
std::expected<std::filesystem::path, std::error_code> GetWith(EUserFolder folder, Variables const& variables);
Variables WithoutOverrides();
} // namespace Tge::IO::Tests
