#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/net/stream_listener.hpp>
#include <tge/net/stream_socket.hpp>
#include <tge/net/udp_socket.hpp>
#include <tge/net/wait_for_readable.hpp>
#include <chrono>

using namespace Tge::Net;
using namespace Tge::Net::Tests;

//////////////////////////////////////////////////////////////////////////
TEST(WaitForReadable, TimesOutWhenNothingArrives)
{
	CUdpSocket socket{};

	ASSERT_TRUE(socket.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	EXPECT_FALSE(WaitForReadable(socket.GetWaitable(), std::chrono::milliseconds{ 20 }).value_or(true));

	socket.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(WaitForReadable, ReportsADatagramThatArrived)
{
	CUdpSocket receiver{};
	CUdpSocket sender{};

	ASSERT_TRUE(receiver.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	ASSERT_TRUE(sender.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	EXPECT_TRUE(sender.Send(receiver.GetLocalEndpoint().value_or(SIpv4Endpoint{}), AsBytes("x")).has_value());
	EXPECT_TRUE(WaitForReadable(receiver.GetWaitable(), Patience).value_or(false));

	sender.Terminate();
	receiver.Terminate();
}

//////////////////////////////////////////////////////////////////////////
TEST(WaitForReadable, ReportsAPeerThatHungUp)
{
	CStreamListener listener{};
	CStreamSocket client{};
	CStreamSocket server{};

	ASSERT_TRUE(listener.Initialize(SIpv4Endpoint{ Loopback, 0 }).has_value());
	ASSERT_TRUE(client.Initialize().has_value());
	ASSERT_TRUE(client.Connect(listener.GetLocalEndpoint().value_or(SIpv4Endpoint{})).has_value());
	ASSERT_TRUE(WaitForReadable(listener.GetWaitable(), Patience).value_or(false));
	ASSERT_TRUE(listener.Accept(server).value_or(false));

	server.Terminate();

	EXPECT_TRUE(WaitForReadable(client.GetWaitable(), Patience).value_or(false));

	client.Terminate();
	listener.Terminate();
}
