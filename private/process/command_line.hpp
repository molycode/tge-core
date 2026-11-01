#pragma once

#include <span>
#include <string>
#include <string_view>

namespace Tge::Process
{
std::string BuildCommandLine(std::string_view program, std::span<std::string const> arguments);
} // namespace Tge::Process
