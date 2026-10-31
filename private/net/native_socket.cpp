#include "native_socket.hpp"
#include <algorithm>

#if defined(TGE_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Net
{
namespace
{
//////////////////////////////////////////////////////////////////////////
int ToTimeoutMs(std::chrono::milliseconds timeout)
{
	return static_cast<int>(std::clamp<std::chrono::milliseconds::rep>(timeout.count(), 0, std::numeric_limits<int>::max()));
}

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
int ToSize(size_t size)
{
	return static_cast<int>(std::min<size_t>(size, static_cast<size_t>(std::numeric_limits<int>::max())));
}

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
}
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
NativeSocket ToNative(uintptr_t socket)
{
	return static_cast<NativeSocket>(socket);
}

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSocketError()
{
#if defined(TGE_PLATFORM_WINDOWS)
	return std::error_code{ WSAGetLastError(), std::system_category() };
#elif defined(TGE_PLATFORM_LINUX)
	return std::error_code{ errno, std::system_category() };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
// The raw codes: on Linux errc::operation_would_block is EAGAIN, which is a real failure of connect.
bool IsConnectInProgress(std::error_code const& error)
{
#if defined(TGE_PLATFORM_WINDOWS)
	return error.category() == std::system_category() && error.value() == WSAEWOULDBLOCK;
#elif defined(TGE_PLATFORM_LINUX)
	return error.category() == std::system_category() && error.value() == EINPROGRESS;
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
std::expected<uintptr_t, std::error_code> OpenSocket(int type)
{
#if defined(TGE_PLATFORM_WINDOWS)
	SOCKET const opened{ WSASocketW(AF_INET, type, 0, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT) };
	std::expected<uintptr_t, std::error_code> result{ std::unexpected{ GetLastSocketError() } };

	if (opened != INVALID_SOCKET)
	{
		std::expected<void, std::error_code> const isNonBlocking{ SetNonBlocking(static_cast<uintptr_t>(opened), true) };

		if (isNonBlocking.has_value())
		{
			result = static_cast<uintptr_t>(opened);
		}
		else
		{
			result = std::unexpected{ isNonBlocking.error() };
			closesocket(opened);
		}
	}

	return result;
#elif defined(TGE_PLATFORM_LINUX)
	int const opened{ ::socket(AF_INET, type | SOCK_NONBLOCK | SOCK_CLOEXEC, 0) };

	return (opened >= 0) ? std::expected<uintptr_t, std::error_code>{ static_cast<uintptr_t>(opened) } : std::unexpected{ GetLastSocketError() };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
std::expected<uintptr_t, std::error_code> AcceptSocket(uintptr_t listener)
{
#if defined(TGE_PLATFORM_WINDOWS)
	SOCKET const accepted{ accept(ToNative(listener), nullptr, nullptr) };
	std::expected<uintptr_t, std::error_code> result{ std::unexpected{ GetLastSocketError() } };

	// What accept4's SOCK_CLOEXEC and SOCK_NONBLOCK give on Linux is set by hand.
	if (accepted != INVALID_SOCKET)
	{
		std::expected<void, std::error_code> prepared{};

		if (SetHandleInformation(reinterpret_cast<HANDLE>(accepted), HANDLE_FLAG_INHERIT, 0) != 0)
		{
			prepared = SetNonBlocking(static_cast<uintptr_t>(accepted), true);
		}
		else
		{
			prepared = std::unexpected{ GetLastSystemError() };
		}

		if (prepared.has_value())
		{
			result = static_cast<uintptr_t>(accepted);
		}
		else
		{
			result = std::unexpected{ prepared.error() };
			closesocket(accepted);
		}
	}

	return result;
#elif defined(TGE_PLATFORM_LINUX)
	int const accepted{ accept4(ToNative(listener), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC) };

	return (accepted >= 0) ? std::expected<uintptr_t, std::error_code>{ static_cast<uintptr_t>(accepted) } : std::unexpected{ GetLastSocketError() };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
void CloseSocket(uintptr_t socket)
{
	if (socket != NoSocket)
	{
#if defined(TGE_PLATFORM_WINDOWS)
		closesocket(ToNative(socket));
#elif defined(TGE_PLATFORM_LINUX)
		close(ToNative(socket));
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
	}
}

//////////////////////////////////////////////////////////////////////////
sockaddr_in ToAddress(SIpv4Endpoint endpoint)
{
	sockaddr_in address{};

	address.sin_family = AF_INET;
	address.sin_port = htons(endpoint.port);
	address.sin_addr.s_addr = htonl(endpoint.address);

	return address;
}

//////////////////////////////////////////////////////////////////////////
SIpv4Endpoint ToEndpoint(sockaddr_in const& address)
{
	return SIpv4Endpoint{ ntohl(address.sin_addr.s_addr), ntohs(address.sin_port) };
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> BindSocket(uintptr_t socket, SIpv4Endpoint local)
{
	sockaddr_in const address{ ToAddress(local) };
	bool const isBound{ bind(ToNative(socket), reinterpret_cast<sockaddr const*>(&address), static_cast<socklen_t>(sizeof(address))) == 0 };

	return isBound ? std::expected<void, std::error_code>{} : std::unexpected{ GetLastSocketError() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<SIpv4Endpoint, std::error_code> GetBoundEndpoint(uintptr_t socket)
{
	sockaddr_in address{};
	socklen_t size{ static_cast<socklen_t>(sizeof(address)) };
	bool const isKnown{ getsockname(ToNative(socket), reinterpret_cast<sockaddr*>(&address), &size) == 0 };

	return isKnown ? std::expected<SIpv4Endpoint, std::error_code>{ ToEndpoint(address) } : std::unexpected{ GetLastSocketError() };
}

//////////////////////////////////////////////////////////////////////////
std::error_code TakePendingError(uintptr_t socket)
{
	int pending{ 0 };
	socklen_t size{ static_cast<socklen_t>(sizeof(pending)) };
	std::error_code result{};

	if (getsockopt(ToNative(socket), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&pending), &size) == 0)
	{
		if (pending != 0)
		{
			result = std::error_code{ pending, std::system_category() };
		}
	}
	else
	{
		result = GetLastSocketError();
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> SetNonBlocking(uintptr_t socket, bool isNonBlocking)
{
#if defined(TGE_PLATFORM_WINDOWS)
	u_long mode{ isNonBlocking ? 1u : 0u };
	bool const isSet{ ioctlsocket(ToNative(socket), FIONBIO, &mode) == 0 };
#elif defined(TGE_PLATFORM_LINUX)
	int const flags{ fcntl(ToNative(socket), F_GETFL, 0) };
	bool const isSet{ flags >= 0 && fcntl(ToNative(socket), F_SETFL, isNonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == 0 };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return isSet ? std::expected<void, std::error_code>{} : std::unexpected{ GetLastSocketError() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<size_t, std::error_code> SendBytes(uintptr_t socket, std::span<std::byte const> bytes, sockaddr_in const* pDestination)
{
#if defined(TGE_PLATFORM_WINDOWS)
	char const* const pData{ reinterpret_cast<char const*>(bytes.data()) };
	int const sent{ (pDestination != nullptr)
		? sendto(ToNative(socket), pData, ToSize(bytes.size()), 0, reinterpret_cast<sockaddr const*>(pDestination), static_cast<int>(sizeof(sockaddr_in)))
		: send(ToNative(socket), pData, ToSize(bytes.size()), 0) };

	return (sent != SOCKET_ERROR) ? std::expected<size_t, std::error_code>{ static_cast<size_t>(sent) } : std::unexpected{ GetLastSocketError() };
#elif defined(TGE_PLATFORM_LINUX)
	// MSG_NOSIGNAL: a peer that is gone must not raise SIGPIPE in the whole process.
	ssize_t const sent{ (pDestination != nullptr)
		? sendto(ToNative(socket), bytes.data(), bytes.size(), MSG_NOSIGNAL, reinterpret_cast<sockaddr const*>(pDestination), sizeof(sockaddr_in))
		: send(ToNative(socket), bytes.data(), bytes.size(), MSG_NOSIGNAL) };

	return (sent >= 0) ? std::expected<size_t, std::error_code>{ static_cast<size_t>(sent) } : std::unexpected{ GetLastSocketError() };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
std::expected<size_t, std::error_code> ReceiveBytes(uintptr_t socket, std::span<std::byte> buffer, sockaddr_in* pSource)
{
	socklen_t sourceSize{ static_cast<socklen_t>(sizeof(sockaddr_in)) };

#if defined(TGE_PLATFORM_WINDOWS)
	char* const pData{ reinterpret_cast<char*>(buffer.data()) };
	int const received{ (pSource != nullptr)
		? recvfrom(ToNative(socket), pData, ToSize(buffer.size()), 0, reinterpret_cast<sockaddr*>(pSource), &sourceSize)
		: recv(ToNative(socket), pData, ToSize(buffer.size()), 0) };

	return (received != SOCKET_ERROR) ? std::expected<size_t, std::error_code>{ static_cast<size_t>(received) } : std::unexpected{ GetLastSocketError() };
#elif defined(TGE_PLATFORM_LINUX)
	ssize_t const received{ (pSource != nullptr)
		? recvfrom(ToNative(socket), buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(pSource), &sourceSize)
		: recv(ToNative(socket), buffer.data(), buffer.size(), 0) };

	return (received >= 0) ? std::expected<size_t, std::error_code>{ static_cast<size_t>(received) } : std::unexpected{ GetLastSocketError() };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
// Hang-up and error count as ready, as with poll() on Linux.
std::expected<bool, std::error_code> PollSocket(uintptr_t socket, EReadiness readiness, std::chrono::milliseconds timeout)
{
	bool const isForReading{ readiness == EReadiness::Readable };

#if defined(TGE_PLATFORM_WINDOWS)
	WSAPOLLFD polled{ ToNative(socket), static_cast<SHORT>(isForReading ? POLLRDNORM : POLLWRNORM), 0 };
	int const numReady{ WSAPoll(&polled, 1, ToTimeoutMs(timeout)) };

	return (numReady != SOCKET_ERROR) ? std::expected<bool, std::error_code>{ numReady > 0 } : std::unexpected{ GetLastSocketError() };
#elif defined(TGE_PLATFORM_LINUX)
	pollfd polled{ ToNative(socket), static_cast<short>(isForReading ? POLLIN : POLLOUT), 0 };
	int const numReady{ poll(&polled, 1, ToTimeoutMs(timeout)) };

	return (numReady >= 0) ? std::expected<bool, std::error_code>{ numReady > 0 } : std::unexpected{ GetLastSocketError() };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}
} // namespace Tge::Net
