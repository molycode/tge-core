#include <gtest/gtest.h>
#include <tge/threading/job_group.hpp>
#include <atomic>

using namespace Tge::Threading;

// Determinism note: every cancellation case cancels BEFORE the jobs it checks are submitted, or
// cancels from WITHIN a running job — so the outcome never depends on pool scheduling/timing.

//////////////////////////////////////////////////////////////////////////
TEST(JobGroupCancellation, RequestedFlagFollowsCancel)
{
	CJobGroup group;

	EXPECT_FALSE(group.IsCancellationRequested());

	group.Cancel();

	EXPECT_TRUE(group.IsCancellationRequested());
}

//////////////////////////////////////////////////////////////////////////
TEST(JobGroupCancellation, UncancelledGroupRunsEveryJob)
{
	CJobGroup group;
	std::atomic<int> ran{ 0 };

	for (int i = 0; i < 64; i++)
	{
		group.SubmitJob([&ran]() { ran.fetch_add(1, std::memory_order_relaxed); });
	}

	group.Wait();

	EXPECT_EQ(ran.load(), 64);
}

//////////////////////////////////////////////////////////////////////////
TEST(JobGroupCancellation, CancelBeforeSubmitSkipsEveryJob)
{
	CJobGroup group;
	std::atomic<int> ran{ 0 };

	// Cancelled up front: each dispatched job is skipped, so Wait() still drains but nothing executes.
	group.Cancel();

	for (int i = 0; i < 64; i++)
	{
		group.SubmitJob([&ran]() { ran.fetch_add(1, std::memory_order_relaxed); });
	}

	group.Wait();

	EXPECT_EQ(ran.load(), 0);
}

//////////////////////////////////////////////////////////////////////////
TEST(JobGroupCancellation, RunningJobSeesOwnCancellation)
{
	CJobGroup group;
	std::atomic<bool> beforeCancel{ true };
	std::atomic<bool> afterCancel{ false };

	group.SubmitJob([&]()
	{
		beforeCancel.store(IsCurrentJobCancelled());
		group.Cancel();
		afterCancel.store(IsCurrentJobCancelled());
	});

	group.Wait();

	EXPECT_FALSE(beforeCancel.load());
	EXPECT_TRUE(afterCancel.load());
}

//////////////////////////////////////////////////////////////////////////
TEST(JobGroupCancellation, NestedGroupInheritsParentCancellation)
{
	CJobGroup parent;
	std::atomic<int> childRan{ 0 };
	std::atomic<bool> childCancelled{ false };

	parent.SubmitJob([&]()
	{
		parent.Cancel();

		// Constructed inside the (now cancelled) parent job — inherits its cancellation with no handle passed.
		CJobGroup child;
		childCancelled.store(child.IsCancellationRequested());

		child.SubmitJob([&childRan]() { childRan.fetch_add(1, std::memory_order_relaxed); });
		child.Wait();
	});

	parent.Wait();

	EXPECT_TRUE(childCancelled.load());
	EXPECT_EQ(childRan.load(), 0);
}

//////////////////////////////////////////////////////////////////////////
TEST(JobGroupCancellation, NestedGroupRunsWhenParentLive)
{
	CJobGroup parent;
	std::atomic<int> childRan{ 0 };

	parent.SubmitJob([&]()
	{
		CJobGroup child;
		child.SubmitJob([&childRan]() { childRan.fetch_add(1, std::memory_order_relaxed); });
		child.Wait();
	});

	parent.Wait();

	EXPECT_EQ(childRan.load(), 1);
}

//////////////////////////////////////////////////////////////////////////
TEST(JobGroupCancellation, NoCurrentJobOutsideAnyJob)
{
	EXPECT_FALSE(IsCurrentJobCancelled());
}
