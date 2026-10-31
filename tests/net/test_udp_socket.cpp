#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/net/udp_socket.hpp>
#include <tge/net/wait_for_readable.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <expected>
#include <span>
#include <system_error>

using namespace Tge::Net;
using namespace Tge::Net::Tests;

namespace
{
// Bound and closed again, so a datagram sent there draws ICMP "port unreachable".
SIpv4Endpoint GetClosedPort()
{
	CUdpSocket probe{};
	SIpv4Endpoint closed{};

	if (probe.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value())
	{
		closed = probe.GetLocalEndpoint().value_or(SIpv4Endpoint{});
	}

	probe.Terminate();

	return closed;
}
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST(UdpSocket, ReceivesWhatAnotherSocketSent)
{
	CUdpSocket receiver{};
	CUdpSocket sender{};
	std::array<std::byte, 64> buffer{};
	SIpv4Endpoint source{};

	ASSERT_TRUE(receiver.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	ASSERT_TRUE(sender.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());

	EXPECT_TRUE(sender.Send(receiver.GetLocalEndpoint().value_or(SIpv4Endpoint{}), AsBytes("hello")).has_value());
	EXPECT_TRUE(WaitForReadable(receiver.GetWaitable(), Patience).value_or(false));

	std::expected<size_t, std::error_code> const received{ receiver.Receive(buffer, source) };

	ASSERT_TRUE(received.has_value());
	EXPECT_EQ(AsText(std::span{ buffer }.first(*received)), "hello");
	EXPECT_EQ(source, sender.GetLocalEndpoint().value_or(SIpv4Endpoint{}));

	sender.Terminate();
	receiver.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(UdpSocket, ReceiveWithNothingWaitingWouldBlock)
{
	CUdpSocket socket{};
	std::array<std::byte, 64> buffer{};
	SIpv4Endpoint source{};

	ASSERT_TRUE(socket.Initialize(SIpv4Endpoint{}).has_value());

	std::expected<size_t, std::error_code> const received{ socket.Receive(buffer, source) };

	ASSERT_FALSE(received.has_value());
	EXPECT_EQ(received.error(), std::errc::operation_would_block);

	socket.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(UdpSocket, ConnectedSocketReportsAClosedPortAsRefused)
{
	SIpv4Endpoint const closed{ GetClosedPort() };
	CUdpSocket socket{};
	std::array<std::byte, 64> buffer{};
	SIpv4Endpoint source{};

	ASSERT_TRUE(socket.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	ASSERT_TRUE(socket.Connect(closed).has_value());

	EXPECT_TRUE(socket.Send(closed, AsBytes("ping")).has_value());
	EXPECT_TRUE(WaitForReadable(socket.GetWaitable(), Patience).value_or(false));

	std::expected<size_t, std::error_code> const received{ socket.Receive(buffer, source) };

	ASSERT_FALSE(received.has_value());
	EXPECT_EQ(received.error(), std::errc::connection_refused);

	socket.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(UdpSocket, UnconnectedSocketWithResetReportsOffIgnoresAClosedPort)
{
	SIpv4Endpoint const closed{ GetClosedPort() };
	CUdpSocket socket{};
	std::array<std::byte, 64> buffer{};
	SIpv4Endpoint source{};

	ASSERT_TRUE(socket.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	ASSERT_TRUE(socket.DisableConnectionResetReports().has_value());

	EXPECT_TRUE(socket.Send(closed, AsBytes("ping")).has_value());
	// Ample time for loopback's ICMP reply to arrive.
	EXPECT_FALSE(WaitForReadable(socket.GetWaitable(), std::chrono::milliseconds{ 200 }).value_or(true));

	std::expected<size_t, std::error_code> const received{ socket.Receive(buffer, source) };

	ASSERT_FALSE(received.has_value());
	EXPECT_EQ(received.error(), std::errc::operation_would_block);

	socket.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(UdpSocket, OversizedDatagramArrivesTruncated)
{
	CUdpSocket receiver{};
	CUdpSocket sender{};
	std::array<std::byte, 4> buffer{};
	SIpv4Endpoint source{};

	ASSERT_TRUE(receiver.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	ASSERT_TRUE(sender.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());

	EXPECT_TRUE(sender.Send(receiver.GetLocalEndpoint().value_or(SIpv4Endpoint{}), AsBytes("0123456789")).has_value());
	EXPECT_TRUE(WaitForReadable(receiver.GetWaitable(), Patience).value_or(false));

	std::expected<size_t, std::error_code> const received{ receiver.Receive(buffer, source) };

	ASSERT_TRUE(received.has_value());
	EXPECT_EQ(AsText(std::span{ buffer }.first(*received)), "0123");
	EXPECT_EQ(source, sender.GetLocalEndpoint().value_or(SIpv4Endpoint{}));

	sender.Terminate();
	receiver.Terminate();
}
