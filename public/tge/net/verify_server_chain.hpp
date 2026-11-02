#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <string_view>
#include <system_error>

namespace Tge::Net
{
std::expected<void, std::error_code> VerifyServerChain(std::span<std::span<std::byte const> const> chain, std::string_view hostName);
} // namespace Tge::Net
