#include <gtest/gtest.h>
#include "system.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <cstdint>
#include <thread>
#include <vector>

using Tge::Events::EventHandle;
using Tge::Events::InvalidEventHandle;
using Tge::Events::Impl::CSystem;

namespace
{
// The dispatcher's own types: naming an engine event would tie this to a module TgeTests does not link.
struct SFirst final
{
};

struct SSecond final
{
};

// Never subscribed anywhere, so its slot is never allocated.
struct SNeverSubscribed final
{
};

struct SPayload final
{
	std::string name;
	uint32_t    value{ 0u };
};

// A subscribed handler runs when its event is emitted, once per emit.
TEST(EventSystemTest, EmitInvokesSubscribedHandler)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });

	system.Emit<SFirst>();
	system.Emit<SFirst>();

	EXPECT_EQ(numCalls, 2u);
}

// Events are independent: emitting one must not reach another's subscribers.
TEST(EventSystemTest, EmitDoesNotInvokeOtherEventsHandlers)
{
	CSystem  system;
	uint32_t numSettled{ 0u };
	uint32_t numQuit{ 0u };

	system.Subscribe<SFirst>([&numSettled]() { ++numSettled; });
	system.Subscribe<SSecond>([&numQuit]() { ++numQuit; });

	system.Emit<SFirst>();

	EXPECT_EQ(numSettled, 1u);
	EXPECT_EQ(numQuit, 0u);
}

// An emit with nobody listening is a no-op, not a crash.
TEST(EventSystemTest, EmitWithoutSubscribersIsHarmless)
{
	CSystem system;

	system.Emit<SNeverSubscribed>();
}

// Unsubscribe stops the handler; the subscription is gone, not merely muted.
TEST(EventSystemTest, UnsubscribedHandlerNoLongerRuns)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	EventHandle const handle{ system.Subscribe<SFirst>([&numCalls]() { ++numCalls; }) };

	system.Emit<SFirst>();
	system.Unsubscribe(handle);
	system.Emit<SFirst>();

	EXPECT_EQ(numCalls, 1u);
}

// Unsubscribing a handle the system never issued -- or the invalid sentinel -- is ignored, and must not
// disturb the subscribers that ARE registered.
TEST(EventSystemTest, UnsubscribeIgnoresUnknownHandle)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });

	system.Unsubscribe(InvalidEventHandle);
	system.Unsubscribe(EventHandle{ 9999u });

	system.Emit<SFirst>();

	EXPECT_EQ(numCalls, 1u);
}

// Handlers run in the order they subscribed. Callers can and do depend on this, so it is a contract rather
// than an accident of the container.
TEST(EventSystemTest, SubscribersRunInSubscriptionOrder)
{
	CSystem               system;
	std::vector<uint32_t> order;

	system.Subscribe<SFirst>([&order]() { order.push_back(0u); });
	system.Subscribe<SFirst>([&order]() { order.push_back(1u); });
	system.Subscribe<SFirst>([&order]() { order.push_back(2u); });

	system.Emit<SFirst>();

	EXPECT_EQ(order, (std::vector<uint32_t>{ 0u, 1u, 2u }));
}

// Every subscriber of an event runs, not just the first or last.
TEST(EventSystemTest, AllSubscribersOfAnEventRun)
{
	CSystem        system;
	uint32_t       numCalls{ 0u };
	constexpr auto NumSubscribers{ 64u };

	for (uint32_t index = 0u; index < NumSubscribers; ++index)
	{
		system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });
	}

	system.Emit<SFirst>();

	EXPECT_EQ(numCalls, NumSubscribers);
}

// A handle is never reissued, so unsubscribing a stale one cannot evict whoever came after it.
TEST(EventSystemTest, StaleHandleCannotEvictALaterSubscriber)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	EventHandle const stale{ system.Subscribe<SFirst>([]() {}) };
	system.Unsubscribe(stale);

	system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });

	system.Unsubscribe(stale);
	system.Emit<SFirst>();

	EXPECT_EQ(numCalls, 1u);
}

// The re-entrancy tests below HANG rather than fail against a dispatcher that locks across dispatch.

// A handler may subscribe. The new handler joins the next dispatch, not the one already in flight.
TEST(EventSystemTest, HandlerMaySubscribeDuringEmit)
{
	CSystem  system;
	uint32_t numLate{ 0u };

	system.Subscribe<SFirst>([&system, &numLate]()
	{
		system.Subscribe<SFirst>([&numLate]() { ++numLate; });
	});

	system.Emit<SFirst>();

	EXPECT_EQ(numLate, 0u) << "a handler subscribed mid-dispatch ran in the dispatch that created it";

	system.Emit<SFirst>();

	EXPECT_EQ(numLate, 1u);
}

// A handler may unsubscribe itself and is not called again, snapshot or not.
TEST(EventSystemTest, HandlerMayUnsubscribeItselfDuringEmit)
{
	CSystem     system;
	uint32_t    numCalls{ 0u };
	EventHandle handle{ InvalidEventHandle };

	handle = system.Subscribe<SFirst>([&system, &numCalls, &handle]()
	{
		++numCalls;
		system.Unsubscribe(handle);
	});

	system.Emit<SFirst>();
	system.Emit<SFirst>();

	EXPECT_EQ(numCalls, 1u);
}

// Unsubscribing ANOTHER handler mid-dispatch takes effect immediately, though it sits in the live snapshot.
TEST(EventSystemTest, HandlerMayUnsubscribeAnotherDuringEmit)
{
	CSystem     system;
	uint32_t    numVictimCalls{ 0u };
	EventHandle victim{ InvalidEventHandle };

	// Subscribed first, so it runs while the victim is still ahead of it in the snapshot being walked.
	system.Subscribe<SFirst>([&system, &victim]() { system.Unsubscribe(victim); });
	victim = system.Subscribe<SFirst>([&numVictimCalls]() { ++numVictimCalls; });

	system.Emit<SFirst>();

	EXPECT_EQ(numVictimCalls, 0u) << "a handler unsubscribed mid-dispatch still ran from the in-flight snapshot";
}

// A handler may emit. Nesting depth is just the stack.
TEST(EventSystemTest, HandlerMayEmitAnotherEventDuringEmit)
{
	CSystem  system;
	uint32_t numQuit{ 0u };

	system.Subscribe<SFirst>([&system]() { system.Emit<SSecond>(); });
	system.Subscribe<SSecond>([&numQuit]() { ++numQuit; });

	system.Emit<SFirst>();

	EXPECT_EQ(numQuit, 1u);
}

// QueueEmit hands the work to whoever drains, so the emitter runs no handler at all.
TEST(EventSystemTest, QueueEmitDoesNotInvokeHandlerUntilDrained)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });

	system.QueueEmit<SFirst>();

	EXPECT_EQ(numCalls, 0u) << "QueueEmit ran the handler on the emitting thread";

	system.DispatchQueued();

	EXPECT_EQ(numCalls, 1u);
}

// A drain delivers every queued event, once each, in the order they were queued.
TEST(EventSystemTest, DispatchQueuedDeliversInQueueOrder)
{
	CSystem               system;
	std::vector<uint32_t> order;

	system.Subscribe<SFirst>([&order]() { order.push_back(0u); });
	system.Subscribe<SSecond>([&order]() { order.push_back(1u); });

	system.QueueEmit<SFirst>();
	system.QueueEmit<SSecond>();
	system.QueueEmit<SFirst>();

	system.DispatchQueued();

	EXPECT_EQ(order, (std::vector<uint32_t>{ 0u, 1u, 0u }));
}

// A drained queue stays drained -- events are consumed, not replayed.
TEST(EventSystemTest, DispatchQueuedConsumesTheQueue)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	system.Subscribe<SFirst>([&numCalls]() { ++numCalls; });

	system.QueueEmit<SFirst>();
	system.DispatchQueued();
	system.DispatchQueued();

	EXPECT_EQ(numCalls, 1u);
}

// A handler that queues lands in the NEXT drain. Otherwise a handler re-queuing its own event would spin the
// drain forever.
TEST(EventSystemTest, EventQueuedByAHandlerDefersToTheNextDrain)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	system.Subscribe<SFirst>([&system, &numCalls]()
	{
		++numCalls;
		system.QueueEmit<SFirst>();
	});

	system.QueueEmit<SFirst>();
	system.DispatchQueued();

	EXPECT_EQ(numCalls, 1u) << "the drain kept consuming what its own handlers queued";

	system.DispatchQueued();

	EXPECT_EQ(numCalls, 2u);
}

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
// A payload reaches the handler unchanged.
TEST(EventSystemTest, EmitDeliversThePayload)
{
	CSystem  system;
	SPayload received;

	system.Subscribe<SPayload>([&received](SPayload const& payload) { received = payload; });

	system.Emit(SPayload{ "sponza", 7u });

	EXPECT_EQ(received.name, "sponza");
	EXPECT_EQ(received.value, 7u);
}

//////////////////////////////////////////////////////////////////////////
// A handler that ignores the payload still compiles and runs.
TEST(EventSystemTest, AHandlerMayIgnoreThePayload)
{
	CSystem  system;
	uint32_t numCalls{ 0u };

	system.Subscribe<SPayload>([&numCalls]() { ++numCalls; });

	system.Emit(SPayload{ "ignored", 1u });

	EXPECT_EQ(numCalls, 1u);
}

//////////////////////////////////////////////////////////////////////////
// Mutates the source rather than destroying it: a short string lives inside the object, so a freed read
// returns the right answer often enough that destruction cannot tell a copy from a reference.
TEST(EventSystemTest, AQueuedPayloadIsCopiedNotReferenced)
{
	CSystem  system;
	SPayload received;

	system.Subscribe<SPayload>([&received](SPayload const& payload) { received = payload; });

	SPayload source{ "curtains", 42u };
	system.QueueEmit(source);

	source.name  = "overwritten";
	source.value = 7u;

	EXPECT_EQ(received.value, 0u) << "QueueEmit ran a handler instead of deferring it";

	system.DispatchQueued();

	EXPECT_EQ(received.name, "curtains") << "the queue referenced the emitter's payload instead of copying it";
	EXPECT_EQ(received.value, 42u);
}

//////////////////////////////////////////////////////////////////////////
// Queued payloads stay matched to their own event rather than to the last one queued.
TEST(EventSystemTest, QueuedPayloadsArriveInOrderWithTheirOwnValues)
{
	CSystem                  system;
	std::vector<std::string> received;

	system.Subscribe<SPayload>([&received](SPayload const& payload) { received.push_back(payload.name); });

	system.QueueEmit(SPayload{ "first", 1u });
	system.QueueEmit(SPayload{ "second", 2u });
	system.QueueEmit(SPayload{ "third", 3u });

	system.DispatchQueued();

	ASSERT_EQ(received.size(), 3u);
	EXPECT_EQ(received[0], "first");
	EXPECT_EQ(received[1], "second");
	EXPECT_EQ(received[2], "third");
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
