#include <gtest/gtest.h>
#include "system.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using Tge::Events::EventHandle;
using Tge::Events::Impl::CSystem;

namespace
{
// The dispatcher's own types: naming an engine event would tie this to a module TgeTests does not link.
struct SFirst final
{
};

struct SPayload final
{
	std::string name;
	uint32_t    value{ 0u };
};

// Determinism note: the concurrency tests assert on counts alone -- never on interleaving, timing, or which
// thread won a race.

// The point of the queue: workers queue from any thread, and every event still arrives exactly once.
TEST(EventSystemTest, QueuedEmitsFromManyThreadsAllArrive)
{
	constexpr uint32_t NumThreads{ 8u };
	constexpr uint32_t NumEmitsPerThread{ 1000u };

	CSystem  system;
	uint32_t numCalls{ 0u };

	// Not atomic on purpose: the handler must only ever run on the draining thread.
	system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });

	std::vector<std::thread> threads;

	for (uint32_t thread = 0u; thread < NumThreads; ++thread)
	{
		threads.emplace_back([&system]()
		{
			for (uint32_t emit = 0u; emit < NumEmitsPerThread; ++emit)
			{
				system.QueueEmit<SFirst>();
			}
		});
	}

	for (std::thread& thread : threads)
	{
		thread.join();
	}

	system.DispatchQueued();

	EXPECT_EQ(numCalls, NumThreads * NumEmitsPerThread);
}

// Draining concurrently with producers loses nothing: what a drain misses is still queued for the next one.
TEST(EventSystemTest, ConcurrentQueueEmitAndDrainLoseNoEvents)
{
	constexpr uint32_t NumThreads{ 4u };
	constexpr uint32_t NumEmitsPerThread{ 2000u };

	CSystem  system;
	uint32_t numCalls{ 0u };

	system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });

	std::vector<std::thread> threads;

	for (uint32_t thread = 0u; thread < NumThreads; ++thread)
	{
		threads.emplace_back([&system]()
		{
			for (uint32_t emit = 0u; emit < NumEmitsPerThread; ++emit)
			{
				system.QueueEmit<SFirst>();
			}
		});
	}

	// Producers push a fixed total, so draining until it arrives terminates without any timing assumption.
	while (numCalls < (NumThreads * NumEmitsPerThread))
	{
		system.DispatchQueued();
	}

	for (std::thread& thread : threads)
	{
		thread.join();
	}

	EXPECT_EQ(numCalls, NumThreads * NumEmitsPerThread);
}

// Concurrent emitters each dispatch to every subscriber -- no emit is lost or doubled under contention.
TEST(EventSystemTest, ConcurrentEmitsInvokeEveryHandler)
{
	constexpr uint32_t NumThreads{ 8u };
	constexpr uint32_t NumEmitsPerThread{ 1000u };
	constexpr uint32_t NumSubscribers{ 4u };

	CSystem               system;
	std::atomic<uint64_t> numCalls{ 0u };

	for (uint32_t index = 0u; index < NumSubscribers; ++index)
	{
		system.Subscribe<SFirst>([&numCalls]() { numCalls.fetch_add(1u, std::memory_order_relaxed); });
	}

	std::vector<std::thread> threads;

	for (uint32_t thread = 0u; thread < NumThreads; ++thread)
	{
		threads.emplace_back([&system]()
		{
			for (uint32_t emit = 0u; emit < NumEmitsPerThread; ++emit)
			{
				system.Emit<SFirst>();
			}
		});
	}

	for (std::thread& thread : threads)
	{
		thread.join();
	}

	EXPECT_EQ(numCalls.load(), uint64_t{ NumThreads } * NumEmitsPerThread * NumSubscribers);
}

// Churn on one thread cannot tear a dispatch on another: a permanent subscriber still runs once per emit.
TEST(EventSystemTest, ChurnDuringConcurrentEmitLeavesPermanentSubscribersIntact)
{
	constexpr uint32_t NumEmits{ 5000u };
	constexpr uint32_t NumPermanent{ 4u };

	CSystem               system;
	std::atomic<uint64_t> numPermanentCalls{ 0u };
	std::atomic<bool>     churning{ true };

	for (uint32_t index = 0u; index < NumPermanent; ++index)
	{
		system.Subscribe<SFirst>([&numPermanentCalls]()
		{
			numPermanentCalls.fetch_add(1u, std::memory_order_relaxed);
		});
	}

	// The churned handlers' own invocation count is a race by construction, so nothing asserts on it.
	std::thread churn([&system, &churning]()
	{
		while (churning.load(std::memory_order_relaxed))
		{
			EventHandle const handle{ system.Subscribe<SFirst>([]() {}) };
			system.Unsubscribe(handle);
		}
	});

	for (uint32_t emit = 0u; emit < NumEmits; ++emit)
	{
		system.Emit<SFirst>();
	}

	churning.store(false, std::memory_order_relaxed);
	churn.join();

	EXPECT_EQ(numPermanentCalls.load(), uint64_t{ NumEmits } * NumPermanent);
}

//////////////////////////////////////////////////////////////////////////
// The cross-thread handoff; ownership is AQueuedPayloadIsCopiedNotReferenced's job.
TEST(EventSystemTest, APayloadQueuedFromAnotherThreadArrivesIntact)
{
	CSystem  system;
	SPayload received;

	system.Subscribe<SPayload>([&received](SPayload const& payload) { received = payload; });

	std::thread worker([&system]()
	{
		system.QueueEmit(SPayload{ "from_worker", 99u });
	});

	worker.join();
	system.DispatchQueued();

	EXPECT_EQ(received.name, "from_worker");
	EXPECT_EQ(received.value, 99u);
}
} // namespace
