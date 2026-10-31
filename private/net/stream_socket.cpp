#include <tge/net/stream_socket.hpp>
#include "native_socket.hpp"
#include <tge/assert.hpp>

#if defined(TGE_PLATFORM_LINUX)
#include <netinet/tcp.h>
#endif // defined(TGE_PLATFORM_LINUX)

namespace Tge::Net
{
//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CStreamSocket::Initialize()
{
	TGE_ASSERT(m_socket == NoSocket, "TCP socket initialized twice");

	std::expected<uintptr_t, std::error_code> const opened{ OpenSocket(SOCK_STREAM) };

	if (opened.has_value())
	{
		m_socket = *opened;
	}

	return opened.has_value() ? std::expected<void, std::error_code>{} : std::unexpected{ opened.error() };
}

//////////////////////////////////////////////////////////////////////////
void CStreamSocket::Terminate()
{
	CloseSocket(m_socket);
	m_socket = NoSocket;
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CStreamSocket::Connect(SIpv4Endpoint peer) const
{
	sockaddr_in const address{ ToAddress(peer) };
	bool const isConnected{ connect(ToNative(m_socket), reinterpret_cast<sockaddr const*>(&address), static_cast<socklen_t>(sizeof(address))) == 0 };
	std::error_code const error{ isConnected ? std::error_code{} : GetLastSocketError() };

	return (isConnected || IsConnectInProgress(error)) ? std::expected<void, std::error_code>{} : std::unexpected{ error };
}

//////////////////////////////////////////////////////////////////////////
// A poll that fails counts as still connecting, to be asked again.
std::expected<bool, std::error_code> CStreamSocket::PollConnected() const
{
	std::expected<bool, std::error_code> result{ false };

	if (PollSocket(m_socket, EReadiness::Writable, std::chrono::milliseconds{ 0 }).value_or(false))
	{
		std::error_code const pending{ TakePendingError(m_socket) };

		result = (pending == std::error_code{}) ? std::expected<bool, std::error_code>{ true } : std::unexpected{ pending };
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
std::expected<size_t, std::error_code> CStreamSocket::Send(std::span<std::byte const> bytes) const
{
	return SendBytes(m_socket, bytes, nullptr);
}

//////////////////////////////////////////////////////////////////////////
std::expected<size_t, std::error_code> CStreamSocket::Receive(std::span<std::byte> buffer) const
{
	std::expected<size_t, std::error_code> received{ ReceiveBytes(m_socket, buffer, nullptr) };

	// Windows says "not connected" where Linux reports a failed connect's error, or EAGAIN while still connecting.
	// Polled before the error is read: a connect that ends in between must not pass for one never made.
	if (!received.has_value() && received.error() == std::errc::not_connected)
	{
		bool const isSettled{ PollSocket(m_socket, EReadiness::Writable, std::chrono::milliseconds{ 0 }).value_or(false) };
		std::error_code const pending{ isSettled ? TakePendingError(m_socket) : std::error_code{} };

		if (!isSettled)
		{
			received = std::unexpected{ std::make_error_code(std::errc::operation_would_block) };
		}
		else if (pending != std::error_code{})
		{
			received = std::unexpected{ pending };
		}
	}

	return received;
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CStreamSocket::SetBlocking() const
{
	return SetNonBlocking(m_socket, false);
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CStreamSocket::SetNoDelay() const
{
	int const isOn{ 1 };
	bool const isSet{ setsockopt(ToNative(m_socket), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char const*>(&isOn), static_cast<socklen_t>(sizeof(isOn))) == 0 };

	return isSet ? std::expected<void, std::error_code>{} : std::unexpected{ GetLastSocketError() };
}

//////////////////////////////////////////////////////////////////////////
Threading::SWaitable CStreamSocket::GetWaitable() const
{
	return Threading::SWaitable{ m_socket, Threading::EWaitableKind::Socket };
}
} // namespace Tge::Net
