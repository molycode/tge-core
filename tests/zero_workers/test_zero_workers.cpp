#include <gtest/gtest.h>
#include <tge/init/init.hpp>
#include <tge/threading/job_group.hpp>
#include <tge/threading/job_system.hpp>

#include <algorithm>
#include <thread>
#include <vector>

using namespace Tge::Threading;

//////////////////////////////////////////////////////////////////////////
TEST(ZeroWorkers, WaitRunsEveryJobOnTheWaitingThread)
{
	ASSERT_EQ(GetNumThreads(), 0u);

	CJobGroup group;
	std::vector<std::thread::id> runners;

	for (int i = 0; i < 64; i++)
	{
		group.SubmitJob([&runners]() { runners.push_back(std::this_thread::get_id()); });
	}

	group.Wait();

	ASSERT_EQ(runners.size(), 64u);
	EXPECT_EQ(std::ranges::count(runners, std::this_thread::get_id()), 64);
}

//////////////////////////////////////////////////////////////////////////
// Negative control: Update leaving the queue alone, and the group never completes.
TEST(ZeroWorkers, APolledGroupCompletesAcrossUpdatesOnTheMainThread)
{
	CJobGroup group;
	std::vector<std::thread::id> runners;

	for (int i = 0; i < 8; i++)
	{
		group.SubmitJob([&runners]() { runners.push_back(std::this_thread::get_id()); });
	}

	ASSERT_FALSE(group.IsComplete()) << "a job ran before anything drained the queue";

	for (int frame = 0; (frame < 64) && !group.IsComplete(); frame++)
	{
		Tge::Update();
	}

	bool const isComplete{ group.IsComplete() };
	auto const numOnMainThread{ std::ranges::count(runners, std::this_thread::get_id()) };

	// A failing run must not leave jobs queued that write to `runners` after it is gone.
	group.Wait();

	EXPECT_TRUE(isComplete);
	EXPECT_EQ(numOnMainThread, 8);
}
