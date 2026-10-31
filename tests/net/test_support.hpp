#pragma once

#include <tge/net/ipv4_endpoint.hpp>
#include <tge/net/stream_socket.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <system_error>

namespace Tge::Net::Tests
{
inline constexpr uint32_t Loopback{ 0x7F000001 };
inline constexpr std::chrono::seconds Patience{ 10 };

std::span<std::byte const> AsBytes(std::string_view text);
std::string_view AsText(std::span<std::byte const> bytes);
std::expected<bool, std::error_code> WaitUntilConnected(CStreamSocket const& socket);
} // namespace Tge::Net::Tests
