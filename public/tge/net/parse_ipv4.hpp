#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace Tge::Net
{
std::optional<uint32_t> ParseIpv4(std::string_view text);
} // namespace Tge::Net
