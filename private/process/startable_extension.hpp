#pragma once

#include <filesystem>
#include <string_view>

namespace Tge::Process
{
bool IsStartableExtension(std::basic_string_view<std::filesystem::path::value_type> extension);
} // namespace Tge::Process
