#include <gtest/gtest.h>
#include <tge/platform.hpp>
#include <tge/threading/event_loop.hpp>
#include <tge/threading/job_system.hpp>
#include <tge/threading/waitable.hpp>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <numeric>
#include <optional>
#include <semaphore>
#include <thread>
#include <vector>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // defined(TGE_PLATFORM_WINDOWS)

using namespace Tge::Threading;
using namespace std::chrono_literals;

namespace
{
constexpr std::chrono::seconds Patience{ 10 };

#if defined(TGE_PLATFORM_WINDOWS)
using NativeSocket = SOCKET;
constexpr NativeSocket NoSocket{ INVALID_SOCKET };
#else
using NativeSocket = int;
constexpr NativeSocket NoSocket{ -1 };
#endif // defined(TGE_PLATFORM_WINDOWS)

// Tests terminate the loop before their locals go; TearDown is only the backstop for one that stopped at an ASSERT.
class CEventLoopTest : public testing::Test
{
protected:

	void SetUp() override
	{
#if defined(TGE_PLATFORM_WINDOWS)
		WSADATA data{};
		ASSERT_EQ(WSAStartup(MAKEWORD(2, 2), &data), 0);
#endif // defined(TGE_PLATFORM_WINDOWS)

		ASSERT_TRUE(m_loop.Initialize("TgeTestLoop"));
	}

	void TearDown() override
	{
		m_loop.Terminate();

#if defined(TGE_PLATFORM_WINDOWS)
		WSACleanup();
#endif // defined(TGE_PLATFORM_WINDOWS)
	}

	CEventLoop m_loop;
};

SWaitable ToWaitable(NativeSocket socket)
{
	return SWaitable{ static_cast<uintptr_t>(socket), EWaitableKind::Socket };
}

void CloseSocket(NativeSocket socket)
{
#if defined(TGE_PLATFORM_WINDOWS)
	closesocket(socket);
#else
	close(socket);
#endif // defined(TGE_PLATFORM_WINDOWS)
}

bool SetNonBlocking(NativeSocket socket)
{
#if defined(TGE_PLATFORM_WINDOWS)
	u_long isNonBlocking{ 1 };
	return ioctlsocket(socket, FIONBIO, &isNonBlocking) == 0;
#else
	int const flags{ fcntl(socket, F_GETFL, 0) };
	return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif // defined(TGE_PLATFORM_WINDOWS)
}

bool BindToLoopback(NativeSocket socket, sockaddr_in& bound)
{
	sockaddr_in loopback{};
	loopback.sin_family = AF_INET;
	loopback.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	socklen_t length{ static_cast<socklen_t>(sizeof(bound)) };

	return bind(socket, reinterpret_cast<sockaddr const*>(&loopback), static_cast<socklen_t>(sizeof(loopback))) == 0
		&& getsockname(socket, reinterpret_cast<sockaddr*>(&bound), &length) == 0;
}

// Two UDP sockets on loopback, connected to each other: a byte the second sends makes the first readable.
bool OpenSocketPair(std::array<NativeSocket, 2>& ends)
{
	std::array<sockaddr_in, 2> bound{};
	bool isOpen{ true };

	for (size_t index{ 0 }; index < ends.size(); ++index)
	{
		ends[index] = socket(AF_INET, SOCK_DGRAM, 0);
		isOpen = isOpen && ends[index] != NoSocket && BindToLoopback(ends[index], bound[index]);
	}

	return isOpen
		&& connect(ends[0], reinterpret_cast<sockaddr const*>(&bound[1]), static_cast<socklen_t>(sizeof(bound[1]))) == 0
		&& connect(ends[1], reinterpret_cast<sockaddr const*>(&bound[0]), static_cast<socklen_t>(sizeof(bound[0]))) == 0;
}

void CloseSocketPair(std::array<NativeSocket, 2> const& ends)
{
	CloseSocket(ends[0]);
	CloseSocket(ends[1]);
}

bool WriteByte(std::array<NativeSocket, 2> const& ends)
{
	char const byte{ 'x' };
	return send(ends[1], &byte, 1, 0) == 1;
}

bool ReadByte(std::array<NativeSocket, 2> const& ends)
{
	char byte{ 0 };
	return recv(ends[0], &byte, 1, 0) == 1;
}
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, PostsRunInPostingOrder)
{
	constexpr uint32_t NumPosts{ 100 };
	std::vector<uint32_t> order{};
	std::binary_semaphore done{ 0 };

	for (uint32_t index{ 0 }; index < NumPosts; ++index)
	{
		m_loop.Post([&order, index]() { order.push_back(index); });
	}

	m_loop.Post([&done]() { done.release(); });

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();

	std::vector<uint32_t> expected(NumPosts);
	std::iota(expected.begin(), expected.end(), 0u);
	EXPECT_EQ(order, expected);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, PostsFromSeveralThreadsAllRun)
{
	constexpr uint32_t NumPosters{ 4 };
	constexpr uint32_t NumPostsEach{ 250 };
	uint32_t numRun{ 0 };
	std::binary_semaphore done{ 0 };
	std::vector<std::thread> posters{};

	for (uint32_t poster{ 0 }; poster < NumPosters; ++poster)
	{
		posters.emplace_back([this, &numRun, &done]()
		{
			InitializeThread();

			for (uint32_t index{ 0 }; index < NumPostsEach; ++index)
			{
				m_loop.Post([&numRun, &done]()
				{
					++numRun;

					if (numRun == NumPosters * NumPostsEach)
					{
						done.release();
					}
				});
			}

			FinalizeThread();
		});
	}

	for (std::thread& poster : posters)
	{
		poster.join();
	}

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();

	EXPECT_EQ(numRun, NumPosters * NumPostsEach);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, CallbacksRunOnTheLoopThread)
{
	bool isOnLoopThread{ false };
	std::binary_semaphore done{ 0 };

	m_loop.Post([this, &isOnLoopThread, &done]()
	{
		isOnLoopThread = m_loop.IsLoopThread();
		done.release();
	});

	EXPECT_FALSE(m_loop.IsLoopThread());
	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();

	EXPECT_TRUE(isOnLoopThread);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, ReadableDescriptorRunsItsCallback)
{
	std::array<NativeSocket, 2> ends{ NoSocket, NoSocket };
	uint32_t numReads{ 0 };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenSocketPair(ends));

	m_loop.Post([this, &ends, &numReads, &done]()
	{
		std::optional<SWatchId> const watch{ m_loop.Watch(ToWaitable(ends[0]), [&ends, &numReads, &done]()
		{
			numReads += ReadByte(ends) ? 1u : 0u;
			done.release();
		}) };

		EXPECT_TRUE(watch.has_value());
	});

	EXPECT_TRUE(WriteByte(ends));
	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	CloseSocketPair(ends);

	EXPECT_EQ(numReads, 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, UnwatchedDescriptorStaysQuiet)
{
	std::array<NativeSocket, 2> ends{ NoSocket, NoSocket };
	bool isWatched{ false };
	bool hasRun{ false };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenSocketPair(ends));

	m_loop.Post([this, &ends, &isWatched, &hasRun]()
	{
		std::optional<SWatchId> const watch{ m_loop.Watch(ToWaitable(ends[0]), [&hasRun]() { hasRun = true; }) };

		isWatched = watch.has_value();

		if (isWatched)
		{
			m_loop.Unwatch(*watch);
		}
	});

	EXPECT_TRUE(WriteByte(ends));

	// Two hops: the second post runs only after a wait that would have reported the socket.
	m_loop.Post([this, &done]() { m_loop.Post([&done]() { done.release(); }); });

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	CloseSocketPair(ends);

	EXPECT_TRUE(isWatched);
	EXPECT_FALSE(hasRun);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, CallbackMayUnwatchItsOwnDescriptor)
{
	std::array<NativeSocket, 2> ends{ NoSocket, NoSocket };
	std::optional<SWatchId> watch{};
	uint32_t numRuns{ 0 };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenSocketPair(ends));

	// Never read, so the socket stays readable: only the Unwatch keeps the callback from running on every pass.
	m_loop.Post([this, &ends, &watch, &numRuns, &done]()
	{
		watch = m_loop.Watch(ToWaitable(ends[0]), [this, &watch, &numRuns, &done]()
		{
			++numRuns;

			if (numRuns == 1u)
			{
				m_loop.Unwatch(*watch);
				m_loop.Post([this, &done]() { m_loop.Post([&done]() { done.release(); }); });
			}
		});
	});

	EXPECT_TRUE(WriteByte(ends));
	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	CloseSocketPair(ends);

	EXPECT_EQ(numRuns, 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, PostThatPostsAgainLeavesRoomForDescriptors)
{
	constexpr uint32_t MaxReposts{ 1000 };
	std::array<NativeSocket, 2> ends{ NoSocket, NoSocket };
	std::function<void()> repost{};
	std::optional<SWatchId> watch{};
	uint32_t numReposts{ 0 };
	uint32_t repostsBeforeWatch{ MaxReposts };
	bool hasWatchRun{ false };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenSocketPair(ends));
	EXPECT_TRUE(WriteByte(ends));

	repost = [this, &repost, &numReposts, &hasWatchRun]()
	{
		++numReposts;

		if (!hasWatchRun && numReposts < MaxReposts)
		{
			m_loop.Post(repost);
		}
	};

	m_loop.Post([this, &ends, &repost, &watch, &numReposts, &repostsBeforeWatch, &hasWatchRun, &done]()
	{
		watch = m_loop.Watch(ToWaitable(ends[0]), [this, &watch, &numReposts, &repostsBeforeWatch, &hasWatchRun, &done]()
		{
			repostsBeforeWatch = numReposts;
			hasWatchRun = true;
			m_loop.Unwatch(*watch);
			done.release();
		});

		EXPECT_TRUE(watch.has_value());
		m_loop.Post(repost);
	});

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	CloseSocketPair(ends);

	EXPECT_LT(repostsBeforeWatch, MaxReposts);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, ReadableSocketAmongManyRunsItsCallback)
{
	// More than one WSAPoll call takes on Windows.
	constexpr size_t NumPairs{ 100 };
	std::array<std::array<NativeSocket, 2>, NumPairs> pairs{};
	std::vector<size_t> ran{};
	bool areOpen{ true };
	std::binary_semaphore done{ 0 };

	for (std::array<NativeSocket, 2>& ends : pairs)
	{
		areOpen = OpenSocketPair(ends) && areOpen;
	}

	ASSERT_TRUE(areOpen);

	m_loop.Post([this, &pairs, &ran, &done]()
	{
		for (size_t index{ 0 }; index < pairs.size(); ++index)
		{
			std::optional<SWatchId> const watch{ m_loop.Watch(ToWaitable(pairs[index][0]), [&pairs, &ran, &done, index]()
			{
				EXPECT_TRUE(ReadByte(pairs[index]));
				ran.push_back(index);

				if (index == NumPairs - 1)
				{
					done.release();
				}
			}) };

			EXPECT_TRUE(watch.has_value());
		}
	});

	EXPECT_TRUE(WriteByte(pairs.back()));
	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();

	for (std::array<NativeSocket, 2> const& ends : pairs)
	{
		CloseSocketPair(ends);
	}

	EXPECT_EQ(ran, (std::vector<size_t>{ NumPairs - 1 }));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, RefusedConnectCountsAsReadable)
{
	// Bound but never listening, so it refuses, and bound throughout, so no other socket takes its port meanwhile.
	NativeSocket const refuser{ socket(AF_INET, SOCK_STREAM, 0) };
	NativeSocket const client{ socket(AF_INET, SOCK_STREAM, 0) };
	sockaddr_in address{};
	std::optional<SWatchId> watch{};
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(refuser != NoSocket && client != NoSocket && BindToLoopback(refuser, address) && SetNonBlocking(client));
	EXPECT_NE(connect(client, reinterpret_cast<sockaddr const*>(&address), static_cast<socklen_t>(sizeof(address))), 0);

	m_loop.Post([this, client, &watch, &done]()
	{
		watch = m_loop.Watch(ToWaitable(client), [this, &watch, &done]()
		{
			m_loop.Unwatch(*watch);
			done.release();
		});

		EXPECT_TRUE(watch.has_value());
	});

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	CloseSocket(client);
	CloseSocket(refuser);
}

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, AutoResetEventRunsItsCallbackOncePerSignal)
{
	HANDLE const event{ CreateEventW(nullptr, FALSE, FALSE, nullptr) };
	uint32_t numRuns{ 0 };
	std::binary_semaphore watched{ 0 };
	std::binary_semaphore ran{ 0 };
	std::binary_semaphore done{ 0 };

	ASSERT_NE(event, nullptr);

	m_loop.Post([this, event, &numRuns, &watched, &ran]()
	{
		std::optional<SWatchId> const watch{ m_loop.Watch(SWaitable{ reinterpret_cast<uintptr_t>(event), EWaitableKind::Event }, [&numRuns, &ran]()
		{
			++numRuns;

			if (numRuns == 1u)
			{
				ran.release();
			}
		}) };

		EXPECT_TRUE(watch.has_value());
		watched.release();
	});

	EXPECT_TRUE(watched.try_acquire_for(Patience));
	EXPECT_NE(SetEvent(event), 0);
	EXPECT_TRUE(ran.try_acquire_for(Patience));

	// Two hops: the second post runs only after a wait that would have reported the event again.
	m_loop.Post([this, &done]() { m_loop.Post([&done]() { done.release(); }); });

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	CloseHandle(event);

	EXPECT_EQ(numRuns, 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, ManualResetEventRunsItsCallbackUntilReset)
{
	constexpr uint32_t NumRunsBeforeReset{ 3 };
	HANDLE const event{ CreateEventW(nullptr, TRUE, FALSE, nullptr) };
	uint32_t numRuns{ 0 };
	std::binary_semaphore done{ 0 };

	ASSERT_NE(event, nullptr);

	m_loop.Post([this, event, &numRuns, &done]()
	{
		std::optional<SWatchId> const watch{ m_loop.Watch(SWaitable{ reinterpret_cast<uintptr_t>(event), EWaitableKind::Event }, [this, event, &numRuns, &done]()
		{
			++numRuns;

			if (numRuns == NumRunsBeforeReset)
			{
				ResetEvent(event);
				m_loop.Post([this, &done]() { m_loop.Post([&done]() { done.release(); }); });
			}
		}) };

		EXPECT_TRUE(watch.has_value());
	});

	EXPECT_NE(SetEvent(event), 0);
	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	CloseHandle(event);

	EXPECT_EQ(numRuns, NumRunsBeforeReset);
}
#endif // defined(TGE_PLATFORM_WINDOWS)

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, TimersRunInDeadlineOrder)
{
	std::vector<int> order{};
	std::binary_semaphore done{ 0 };

	m_loop.Post([this, &order, &done]()
	{
		auto const now{ std::chrono::steady_clock::now() };

		m_loop.ScheduleAt(now + 30ms, [&order, &done]() { order.push_back(30); done.release(); });
		m_loop.ScheduleAt(now + 10ms, [&order]() { order.push_back(10); });
		m_loop.ScheduleAt(now + 20ms, [&order]() { order.push_back(20); });
	});

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();

	EXPECT_EQ(order, (std::vector<int>{ 10, 20, 30 }));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, CancelledTimerNeverRuns)
{
	bool hasCancelledRun{ false };
	std::binary_semaphore done{ 0 };

	m_loop.Post([this, &hasCancelledRun, &done]()
	{
		auto const now{ std::chrono::steady_clock::now() };
		STimerId const cancelled{ m_loop.ScheduleAt(now + 10ms, [&hasCancelledRun]() { hasCancelledRun = true; }) };

		m_loop.ScheduleAt(now + 20ms, [&done]() { done.release(); });
		m_loop.CancelTimer(cancelled);
	});

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();

	EXPECT_FALSE(hasCancelledRun);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, TerminateDropsPendingTimer)
{
	bool hasRun{ false };
	std::binary_semaphore scheduled{ 0 };

	m_loop.Post([this, &hasRun, &scheduled]()
	{
		m_loop.ScheduleAt(std::chrono::steady_clock::now() + 1h, [&hasRun]() { hasRun = true; });
		scheduled.release();
	});

	EXPECT_TRUE(scheduled.try_acquire_for(Patience));
	m_loop.Terminate();

	EXPECT_FALSE(hasRun);
}
