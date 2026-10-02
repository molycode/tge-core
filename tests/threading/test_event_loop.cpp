#include <gtest/gtest.h>
#include <tge/platform.hpp>

#if defined(TGE_PLATFORM_LINUX)
#include <tge/threading/event_loop.hpp>
#include <tge/threading/job_system.hpp>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <numeric>
#include <optional>
#include <semaphore>
#include <thread>
#include <vector>
#include <unistd.h>

using namespace Tge::Threading;
using namespace std::chrono_literals;

namespace
{
constexpr std::chrono::seconds Patience{ 10 };

// Tests terminate the loop before their locals go; TearDown is only the backstop for one that stopped at an ASSERT.
class CEventLoopTest : public testing::Test
{
protected:

	void SetUp() override
	{
		ASSERT_TRUE(m_loop.Initialize("TgeTestLoop"));
	}

	void TearDown() override
	{
		m_loop.Terminate();
	}

	CEventLoop m_loop;
};

bool OpenPipe(std::array<int, 2>& ends)
{
	return pipe(ends.data()) == 0;
}

void ClosePipe(std::array<int, 2> const& ends)
{
	close(ends[0]);
	close(ends[1]);
}

bool WriteByte(std::array<int, 2> const& ends)
{
	char const byte{ 'x' };
	return write(ends[1], &byte, 1) == 1;
}

bool ReadByte(std::array<int, 2> const& ends)
{
	char byte{ 0 };
	return read(ends[0], &byte, 1) == 1;
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
	std::array<int, 2> pipeEnds{ -1, -1 };
	uint32_t numReads{ 0 };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenPipe(pipeEnds));

	m_loop.Post([this, &pipeEnds, &numReads, &done]()
	{
		std::optional<SWatchId> const watch{ m_loop.Watch(pipeEnds[0], [&pipeEnds, &numReads, &done]()
		{
			numReads += ReadByte(pipeEnds) ? 1u : 0u;
			done.release();
		}) };

		EXPECT_TRUE(watch.has_value());
	});

	EXPECT_TRUE(WriteByte(pipeEnds));
	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	ClosePipe(pipeEnds);

	EXPECT_EQ(numReads, 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, UnwatchedDescriptorStaysQuiet)
{
	std::array<int, 2> pipeEnds{ -1, -1 };
	bool isWatched{ false };
	bool hasRun{ false };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenPipe(pipeEnds));

	m_loop.Post([this, &pipeEnds, &isWatched, &hasRun]()
	{
		std::optional<SWatchId> const watch{ m_loop.Watch(pipeEnds[0], [&hasRun]() { hasRun = true; }) };

		isWatched = watch.has_value();

		if (isWatched)
		{
			m_loop.Unwatch(*watch);
		}
	});

	EXPECT_TRUE(WriteByte(pipeEnds));

	// Two hops: the second post runs only after a wait that would have reported the pipe.
	m_loop.Post([this, &done]() { m_loop.Post([&done]() { done.release(); }); });

	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	ClosePipe(pipeEnds);

	EXPECT_TRUE(isWatched);
	EXPECT_FALSE(hasRun);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, CallbackMayUnwatchItsOwnDescriptor)
{
	std::array<int, 2> pipeEnds{ -1, -1 };
	std::optional<SWatchId> watch{};
	uint32_t numRuns{ 0 };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenPipe(pipeEnds));

	// Never read, so the pipe stays readable: only the Unwatch keeps the callback from running on every pass.
	m_loop.Post([this, &pipeEnds, &watch, &numRuns, &done]()
	{
		watch = m_loop.Watch(pipeEnds[0], [this, &watch, &numRuns, &done]()
		{
			++numRuns;

			if (numRuns == 1u)
			{
				m_loop.Unwatch(*watch);
				m_loop.Post([this, &done]() { m_loop.Post([&done]() { done.release(); }); });
			}
		});
	});

	EXPECT_TRUE(WriteByte(pipeEnds));
	EXPECT_TRUE(done.try_acquire_for(Patience));
	m_loop.Terminate();
	ClosePipe(pipeEnds);

	EXPECT_EQ(numRuns, 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CEventLoopTest, PostThatPostsAgainLeavesRoomForDescriptors)
{
	constexpr uint32_t MaxReposts{ 1000 };
	std::array<int, 2> pipeEnds{ -1, -1 };
	std::function<void()> repost{};
	std::optional<SWatchId> watch{};
	uint32_t numReposts{ 0 };
	uint32_t repostsBeforeWatch{ MaxReposts };
	bool hasWatchRun{ false };
	std::binary_semaphore done{ 0 };

	ASSERT_TRUE(OpenPipe(pipeEnds));
	EXPECT_TRUE(WriteByte(pipeEnds));

	repost = [this, &repost, &numReposts, &hasWatchRun]()
	{
		++numReposts;

		if (!hasWatchRun && numReposts < MaxReposts)
		{
			m_loop.Post(repost);
		}
	};

	m_loop.Post([this, &pipeEnds, &repost, &watch, &numReposts, &repostsBeforeWatch, &hasWatchRun, &done]()
	{
		watch = m_loop.Watch(pipeEnds[0], [this, &watch, &numReposts, &repostsBeforeWatch, &hasWatchRun, &done]()
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
	ClosePipe(pipeEnds);

	EXPECT_LT(repostsBeforeWatch, MaxReposts);
}

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
#endif // defined(TGE_PLATFORM_LINUX)
