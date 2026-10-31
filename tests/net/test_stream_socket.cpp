#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/net/stream_listener.hpp>
#include <tge/net/stream_socket.hpp>
#include <tge/net/wait_for_readable.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <expected>
#include <span>
#include <system_error>
#include <thread>

using namespace Tge::Net;
using namespace Tge::Net::Tests;

namespace
{
// Listened on and closed again, so a connect there is refused.
SIpv4Endpoint GetClosedPort()
{
	CStreamListener probe{};
	SIpv4Endpoint closed{};

	if (probe.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value())
	{
		closed = probe.GetLocalEndpoint().value_or(SIpv4Endpoint{});
	}

	probe.Terminate();

	return closed;
}

bool Connect(CStreamListener& listener, CStreamSocket& client, CStreamSocket& server)
{
	return listener.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value()
		&& client.Initialize().has_value()
		&& client.Connect(listener.GetLocalEndpoint().value_or(SIpv4Endpoint{})).has_value()
		&& WaitForReadable(listener.GetWaitable(), Patience).value_or(false)
		&& listener.Accept(server).value_or(false)
		&& WaitUntilConnected(client).value_or(false);
}

std::expected<size_t, std::error_code> ReceiveWhenSettled(CStreamSocket const& socket, std::span<std::byte> buffer)
{
	auto const deadline{ std::chrono::steady_clock::now() + Patience };
	std::expected<size_t, std::error_code> received{ socket.Receive(buffer) };

	while (!received.has_value() && received.error() == std::errc::operation_would_block && std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		received = socket.Receive(buffer);
	}

	return received;
}
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST(StreamSocket, ExchangesBytesWithAnAcceptedConnection)
{
	CStreamListener listener{};
	CStreamSocket client{};
	CStreamSocket server{};
	std::array<std::byte, 64> buffer{};

	ASSERT_TRUE(Connect(listener, client, server));
	EXPECT_EQ(client.Send(AsBytes("ping")).value_or(0), 4u);
	EXPECT_TRUE(WaitForReadable(server.GetWaitable(), Patience).value_or(false));

	std::expected<size_t, std::error_code> const received{ server.Receive(buffer) };

	ASSERT_TRUE(received.has_value());
	EXPECT_EQ(AsText(std::span{ buffer }.first(*received)), "ping");

	server.Terminate();
	client.Terminate();
	listener.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(StreamSocket, PeerCloseReadsAsZeroBytes)
{
	CStreamListener listener{};
	CStreamSocket client{};
	CStreamSocket server{};
	std::array<std::byte, 64> buffer{};

	ASSERT_TRUE(Connect(listener, client, server));
	server.Terminate();

	std::expected<size_t, std::error_code> const received{ ReceiveWhenSettled(client, buffer) };

	ASSERT_TRUE(received.has_value());
	EXPECT_EQ(*received, 0u);

	client.Terminate();
	listener.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(StreamSocket, AcceptWithNothingPendingReturnsFalse)
{
	CStreamListener listener{};
	CStreamSocket server{};

	ASSERT_TRUE(listener.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());

	std::expected<bool, std::error_code> const accepted{ listener.Accept(server) };

	ASSERT_TRUE(accepted.has_value());
	EXPECT_FALSE(*accepted);

	listener.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(StreamSocket, RefusedConnectIsReportedByPollConnected)
{
	SIpv4Endpoint const closed{ GetClosedPort() };
	CStreamSocket client{};

	ASSERT_TRUE(client.Initialize().has_value());

	std::expected<void, std::error_code> const connecting{ client.Connect(closed) };
	std::expected<bool, std::error_code> const connected{ connecting.has_value() ? WaitUntilConnected(client)
		: std::expected<bool, std::error_code>{ std::unexpected{ connecting.error() } } };

	ASSERT_FALSE(connected.has_value());
	EXPECT_EQ(connected.error(), std::errc::connection_refused);

	client.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(StreamSocket, RefusedConnectIsReportedByReceive)
{
	SIpv4Endpoint const closed{ GetClosedPort() };
	CStreamSocket client{};
	std::array<std::byte, 64> buffer{};

	ASSERT_TRUE(client.Initialize().has_value());

	std::expected<void, std::error_code> const connecting{ client.Connect(closed) };
	std::expected<size_t, std::error_code> const received{ connecting.has_value() ? ReceiveWhenSettled(client, buffer)
		: std::expected<size_t, std::error_code>{ std::unexpected{ connecting.error() } } };

	ASSERT_FALSE(received.has_value());
	EXPECT_EQ(received.error(), std::errc::connection_refused);

	client.Terminate();
}
