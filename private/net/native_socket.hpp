#pragma once

#include "readiness.hpp"
#include <tge/net/ipv4_endpoint.hpp>
#include <tge/platform.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <system_error>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <netinet/in.h>
#include <sys/socket.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Net
{
#if defined(TGE_PLATFORM_WINDOWS)
using NativeSocket = SOCKET;
#elif defined(TGE_PLATFORM_LINUX)
using NativeSocket = int;
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

inline constexpr uintptr_t NoSocket{ std::numeric_limits<uintptr_t>::max() };

NativeSocket ToNative(uintptr_t socket);
std::error_code GetLastSocketError();
bool IsConnectInProgress(std::error_code const& error);

std::expected<uintptr_t, std::error_code> OpenSocket(int type);
std::expected<uintptr_t, std::error_code> AcceptSocket(uintptr_t listener);
void CloseSocket(uintptr_t socket);

sockaddr_in ToAddress(SIpv4Endpoint endpoint);
SIpv4Endpoint ToEndpoint(sockaddr_in const& address);
std::expected<void, std::error_code> BindSocket(uintptr_t socket, SIpv4Endpoint local);
std::expected<SIpv4Endpoint, std::error_code> GetBoundEndpoint(uintptr_t socket);
std::error_code TakePendingError(uintptr_t socket);
std::expected<void, std::error_code> SetNonBlocking(uintptr_t socket, bool isNonBlocking);

std::expected<size_t, std::error_code> SendBytes(uintptr_t socket, std::span<std::byte const> bytes, sockaddr_in const* pDestination);
std::expected<size_t, std::error_code> ReceiveBytes(uintptr_t socket, std::span<std::byte> buffer, sockaddr_in* pSource);
std::expected<bool, std::error_code> PollSocket(uintptr_t socket, EReadiness readiness, std::chrono::milliseconds timeout);
} // namespace Tge::Net
