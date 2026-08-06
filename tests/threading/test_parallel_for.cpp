#include <gtest/gtest.h>
#include <tge/threading/parallel_for.hpp>
#include <tge/threading/job_group.hpp>
#include <tge/threading/job_system.hpp>
#include <atomic>
#include <thread>
#include <vector>

using namespace Tge::Threading;

// Determinism note: no test asserts on timing or on which thread ran which chunk. The serial cases pin
// the grain above the item count, and the cancellation case cancels from WITHIN the running job — so no
// outcome depends on pool scheduling.

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForRange, VisitsEveryIndexExactlyOnce)
{
	// uint32_t, never vector<bool>: the parallel writes below must land in distinct memory locations.
	std::vector<uint32_t> visits(4096u, 0u);

	ParallelFor(0u, static_cast<uint32_t>(visits.size()), [&visits](uint32_t index)
	{
		visits[index]++;
	}, 1u);

	bool everyIndexVisitedOnce{ true };

	for (uint32_t const count : visits)
	{
		everyIndexVisitedOnce = everyIndexVisitedOnce && (count == 1u);
	}

	EXPECT_TRUE(everyIndexVisitedOnce);
}

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForRange, EmptyRangeInvokesNothing)
{
	std::atomic<uint32_t> ran{ 0u };

	ParallelFor(7u, 7u, [&ran](uint32_t) { ran.fetch_add(1u, std::memory_order_relaxed); }, 1u);

	EXPECT_EQ(ran.load(), 0u);
}

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForRange, HonoursANonZeroBegin)
{
	std::atomic<uint64_t> sum{ 0u };

	ParallelFor(100u, 200u, [&sum](uint32_t index)
	{
		sum.fetch_add(index, std::memory_order_relaxed);
	}, 1u);

	// Sum of [100, 200) == 14950.
	EXPECT_EQ(sum.load(), 14950u);
}

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForChunking, SubGrainRangeStaysOnTheCallingThread)
{
	std::thread::id const caller{ std::this_thread::get_id() };
	std::atomic<uint32_t> offThread{ 0u };

	// Grain above the item count: the range is not worth splitting, so the caller runs it inline.
	ParallelFor(0u, 8u, [&caller, &offThread](uint32_t)
	{
		if (std::this_thread::get_id() != caller)
		{
			offThread.fetch_add(1u, std::memory_order_relaxed);
		}
	}, 64u);

	EXPECT_EQ(offThread.load(), 0u);
}

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForChunking, GrainCapsTheChunkCount)
{
	EXPECT_EQ(ComputeParallelChunkCount(0u, 64u), 0u);
	EXPECT_EQ(ComputeParallelChunkCount(10u, 64u), 1u);
	EXPECT_EQ(ComputeParallelChunkCount(64u, 64u), 1u);
	EXPECT_EQ(ComputeParallelChunkCount(65u, 64u), 2u);
}

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForChunking, ChunkCountSaturatesAtOnePerWorker)
{
	uint32_t const budget{ ComputeParallelChunkCount(1000000u, 1u) };

	EXPECT_EQ(static_cast<size_t>(budget), GetNumThreads());
}

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForNesting, NestedCallInsideAJobCompletes)
{
	CJobGroup group;
	std::atomic<uint32_t> total{ 0u };

	// If a nested ParallelFor could not run its chunks on the job's own thread, this would hang, not fail.
	group.SubmitJob([&total]()
	{
		ParallelFor(0u, 1024u, [&total](uint32_t)
		{
			total.fetch_add(1u, std::memory_order_relaxed);
		}, 1u);
	});

	group.Wait();

	EXPECT_EQ(total.load(), 1024u);
}

//////////////////////////////////////////////////////////////////////////
TEST(ParallelForCancellation, CancelledAncestorSkipsEveryBody)
{
	CJobGroup group;
	std::atomic<uint32_t> ran{ 0u };

	// Cancelling from inside the running job makes the nested group inherit a cancelled parent.
	group.SubmitJob([&group, &ran]()
	{
		group.Cancel();

		ParallelFor(0u, 4096u, [&ran](uint32_t)
		{
			ran.fetch_add(1u, std::memory_order_relaxed);
		}, 1u);
	});

	group.Wait();

	EXPECT_EQ(ran.load(), 0u);
}
