#include <tge/net/udp_socket.hpp>
#include "native_socket.hpp"
#include <tge/assert.hpp>

#if defined(TGE_PLATFORM_WINDOWS)
#include <mswsock.h>
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Net
{
namespace
{
//////////////////////////////////////////////////////////////////////////
// On UDP Windows reports ICMP "port unreachable" as a reset, which Linux calls a refusal; Linux never resets UDP.
std::error_code ToUdpError(std::error_code const& error)
{
	return (error == std::errc::connection_reset) ? std::make_error_code(std::errc::connection_refused) : error;
}

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> TurnOffReports(uintptr_t socket, DWORD control)
{
	BOOL isReporting{ FALSE };
	DWORD numReturned{ 0 };
	bool const isTurnedOff{ WSAIoctl(ToNative(socket), control, &isReporting, sizeof(isReporting), nullptr, 0, &numReturned, nullptr, nullptr) == 0 };

	return isTurnedOff ? std::expected<void, std::error_code>{} : std::unexpected{ GetLastSocketError() };
}
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CUdpSocket::Initialize(SIpv4Endpoint local)
{
	TGE_ASSERT(m_socket == NoSocket, "UDP socket initialized twice");

	std::expected<uintptr_t, std::error_code> const opened{ OpenSocket(SOCK_DGRAM) };
	std::expected<void, std::error_code> result{};

	if (opened.has_value())
	{
		// Bound at once: Windows fails a receive on an unbound socket, and the owner may watch it before its first send.
		result = BindSocket(*opened, local);

#if defined(TGE_PLATFORM_WINDOWS)
		// Linux ignores ICMP "time exceeded" on UDP.
		if (result.has_value())
		{
			result = TurnOffReports(*opened, SIO_UDP_NETRESET);
		}
#endif // defined(TGE_PLATFORM_WINDOWS)

		if (result.has_value())
		{
			m_socket = *opened;
		}
		else
		{
			CloseSocket(*opened);
		}
	}
	else
	{
		result = std::unexpected{ opened.error() };
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CUdpSocket::Terminate()
{
	CloseSocket(m_socket);
	m_socket = NoSocket;
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CUdpSocket::SetReceiveBufferSize(uint32_t numBytes) const
{
	int const size{ static_cast<int>(numBytes) };
	bool const isSet{ setsockopt(ToNative(m_socket), SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char const*>(&size), static_cast<socklen_t>(sizeof(size))) == 0 };

	return isSet ? std::expected<void, std::error_code>{} : std::unexpected{ GetLastSocketError() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CUdpSocket::DisableConnectionResetReports() const
{
#if defined(TGE_PLATFORM_WINDOWS)
	return TurnOffReports(m_socket, SIO_UDP_CONNRESET);
#elif defined(TGE_PLATFORM_LINUX)
	return {};
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CUdpSocket::Connect(SIpv4Endpoint peer) const
{
	sockaddr_in const address{ ToAddress(peer) };
	bool const isConnected{ connect(ToNative(m_socket), reinterpret_cast<sockaddr const*>(&address), static_cast<socklen_t>(sizeof(address))) == 0 };

	return isConnected ? std::expected<void, std::error_code>{} : std::unexpected{ GetLastSocketError() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CUdpSocket::Send(SIpv4Endpoint destination, std::span<std::byte const> datagram) const
{
	sockaddr_in const address{ ToAddress(destination) };
	std::expected<size_t, std::error_code> const sent{ SendBytes(m_socket, datagram, &address) };

	return sent.has_value() ? std::expected<void, std::error_code>{} : std::unexpected{ ToUdpError(sent.error()) };
}

//////////////////////////////////////////////////////////////////////////
std::expected<size_t, std::error_code> CUdpSocket::Receive(std::span<std::byte> buffer, SIpv4Endpoint& source) const
{
	sockaddr_in sender{};
	std::expected<size_t, std::error_code> received{ ReceiveBytes(m_socket, buffer, &sender) };

	// Windows fails a datagram larger than the buffer, which Linux hands over truncated.
	if (!received.has_value() && received.error() == std::errc::message_size)
	{
		received = buffer.size();
	}

	if (received.has_value())
	{
		source = ToEndpoint(sender);
	}
	else
	{
		received = std::unexpected{ ToUdpError(received.error()) };
	}

	return received;
}

//////////////////////////////////////////////////////////////////////////
std::expected<SIpv4Endpoint, std::error_code> CUdpSocket::GetLocalEndpoint() const
{
	return GetBoundEndpoint(m_socket);
}

//////////////////////////////////////////////////////////////////////////
Threading::SWaitable CUdpSocket::GetWaitable() const
{
	return Threading::SWaitable{ m_socket, Threading::EWaitableKind::Socket };
}
} // namespace Tge::Net
