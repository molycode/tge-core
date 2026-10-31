#include <tge/net/stream_listener.hpp>
#include "native_socket.hpp"
#include <tge/assert.hpp>
#include <tge/net/stream_socket.hpp>

namespace Tge::Net
{
//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CStreamListener::Initialize(SIpv4Endpoint local)
{
	TGE_ASSERT(m_socket == NoSocket, "Listener initialized twice");

	std::expected<uintptr_t, std::error_code> const opened{ OpenSocket(SOCK_STREAM) };
	std::expected<void, std::error_code> result{};

	if (opened.has_value())
	{
		result = BindSocket(*opened, local);

		if (result.has_value() && listen(ToNative(*opened), SOMAXCONN) != 0)
		{
			result = std::unexpected{ GetLastSocketError() };
		}

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
void CStreamListener::Terminate()
{
	CloseSocket(m_socket);
	m_socket = NoSocket;
}

//////////////////////////////////////////////////////////////////////////
std::expected<bool, std::error_code> CStreamListener::Accept(CStreamSocket& connection) const
{
	TGE_ASSERT(connection.m_socket == NoSocket, "Accept into a socket that is open");

	std::expected<uintptr_t, std::error_code> const accepted{ AcceptSocket(m_socket) };
	std::expected<bool, std::error_code> result{ false };

	if (accepted.has_value())
	{
		connection.m_socket = *accepted;
		result = true;
	}
	else if (accepted.error() != std::errc::operation_would_block)
	{
		result = std::unexpected{ accepted.error() };
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
std::expected<SIpv4Endpoint, std::error_code> CStreamListener::GetLocalEndpoint() const
{
	return GetBoundEndpoint(m_socket);
}

//////////////////////////////////////////////////////////////////////////
Threading::SWaitable CStreamListener::GetWaitable() const
{
	return Threading::SWaitable{ m_socket, Threading::EWaitableKind::Socket };
}
} // namespace Tge::Net
