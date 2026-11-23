#include <gtest/gtest.h>
#include <tge/threading/job_group.hpp>
#include <tge/threading/job_system.hpp>
#include "threading/job_scheduler.hpp"

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
TEST(ZeroWorkers, APoolWithoutWorkersIsHandedNoJobs)
{
	CJobGroup group;

	for (int i = 0; i < 64; i++)
	{
		group.SubmitJob([]() {});
	}

	group.Wait();

	// Whatever the pool is handed, no thread ever runs or frees, so its queue is the only place the leak shows.
	EXPECT_EQ(gJobScheduler->GetPendingJobCount(), 0u);
}
