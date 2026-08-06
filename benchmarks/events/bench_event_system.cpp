#include "system.hpp"

#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <vector>

using namespace Tge::Events;

namespace
{
// The dispatcher's own type: naming an engine event would tie this to a module core sits below.
struct SBenchEvent final
{
};

// To 4096: a shipping game subscribes per entity, not the four hardcoded callbacks the engine has today.
constexpr int64_t MinSubscribers{ 1 };
constexpr int64_t MaxSubscribers{ 4096 };

// A barrier, not a counter -- shared state here would measure the concurrent case's contention, not Emit's.
void NoopHandler()
{
	benchmark::ClobberMemory();
}

//////////////////////////////////////////////////////////////////////////
Impl::CSystem              gConcurrentSystem;
std::vector<EventHandle>   gConcurrentHandles;

void SetupConcurrent(benchmark::State const& state)
{
	for (int64_t index = 0; index < state.range(0); ++index)
	{
		gConcurrentHandles.push_back(gConcurrentSystem.Subscribe<SBenchEvent>(NoopHandler));
	}
}

void TeardownConcurrent(benchmark::State const&)
{
	// The queued run leaves a backlog; drain it here so it neither grows without bound nor lands on the clock.
	gConcurrentSystem.DispatchQueued();

	for (EventHandle const handle : gConcurrentHandles)
	{
		gConcurrentSystem.Unsubscribe(handle);
	}

	gConcurrentHandles.clear();
}

//////////////////////////////////////////////////////////////////////////
// The cost of merely HAVING an event: most events have no subscriber most of the time.
void BM_EmitNoSubscribers(benchmark::State& state)
{
	Impl::CSystem system;

	for (auto _ : state)
	{
		system.Emit<SBenchEvent>();
	}

	state.SetItemsProcessed(state.iterations());
}

//////////////////////////////////////////////////////////////////////////
// The engine's shape today: one subscriber per event.
void BM_EmitOneSubscriber(benchmark::State& state)
{
	Impl::CSystem system;
	system.Subscribe<SBenchEvent>(NoopHandler);

	for (auto _ : state)
	{
		system.Emit<SBenchEvent>();
	}

	state.SetItemsProcessed(state.iterations());
}

//////////////////////////////////////////////////////////////////////////
// Dispatch cost against subscriber count -- where a per-handler hash lookup would show up.
void BM_EmitManySubscribers(benchmark::State& state)
{
	uint32_t const numSubscribers{ static_cast<uint32_t>(state.range(0)) };
	Impl::CSystem  system;

	for (uint32_t index = 0u; index < numSubscribers; ++index)
	{
		system.Subscribe<SBenchEvent>(NoopHandler);
	}

	for (auto _ : state)
	{
		system.Emit<SBenchEvent>();
	}

	state.SetItemsProcessed(state.iterations() * numSubscribers);
}

//////////////////////////////////////////////////////////////////////////
// How badly concurrent emitters serialise -- the prerequisite for ever emitting from a job-pool worker.
void BM_EmitConcurrent(benchmark::State& state)
{
	for (auto _ : state)
	{
		gConcurrentSystem.Emit<SBenchEvent>();
	}

	state.SetItemsProcessed(state.iterations());
}

//////////////////////////////////////////////////////////////////////////
// What the EMITTER pays: the push alone. The subscribers exist to prove the cost does not depend on them --
// the emitter runs none of them. Drained off the clock, purely to bound the queue.
void BM_QueueEmit(benchmark::State& state)
{
	constexpr uint64_t DrainInterval{ 65536u };

	Impl::CSystem system;
	uint64_t      numQueued{ 0u };

	for (int64_t index = 0; index < state.range(0); ++index)
	{
		system.Subscribe<SBenchEvent>(NoopHandler);
	}

	for (auto _ : state)
	{
		system.QueueEmit<SBenchEvent>();
		++numQueued;

		if (numQueued == DrainInterval)
		{
			state.PauseTiming();
			system.DispatchQueued();
			numQueued = 0u;
			state.ResumeTiming();
		}
	}

	state.SetItemsProcessed(state.iterations());
}

//////////////////////////////////////////////////////////////////////////
// The worker's view: N threads notifying at once. Emit serialises them behind each other's handlers; this
// path only ever contends on a push.
void BM_QueueEmitConcurrent(benchmark::State& state)
{
	for (auto _ : state)
	{
		gConcurrentSystem.QueueEmit<SBenchEvent>();
	}

	state.SetItemsProcessed(state.iterations());
}

//////////////////////////////////////////////////////////////////////////
// Level load: N entities subscribe in a burst. A snapshot rebuilt on EVERY mutation is O(N^2) here.
void BM_SubscribeBurst(benchmark::State& state)
{
	uint32_t const numSubscribers{ static_cast<uint32_t>(state.range(0)) };

	// Teardown of the N std::functions is timed too -- a level load pays it.
	for (auto _ : state)
	{
		Impl::CSystem system;

		for (uint32_t index = 0u; index < numSubscribers; ++index)
		{
			system.Subscribe<SBenchEvent>(NoopHandler);
		}

		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * numSubscribers);
}

//////////////////////////////////////////////////////////////////////////
// Steady state: entities spawn and despawn while N others stay subscribed -- the path a snapshot slows down.
void BM_SubscribeUnsubscribeChurn(benchmark::State& state)
{
	uint32_t const numSubscribers{ static_cast<uint32_t>(state.range(0)) };
	Impl::CSystem  system;

	for (uint32_t index = 0u; index < numSubscribers; ++index)
	{
		system.Subscribe<SBenchEvent>(NoopHandler);
	}

	for (auto _ : state)
	{
		EventHandle const handle{ system.Subscribe<SBenchEvent>(NoopHandler) };
		system.Unsubscribe(handle);
	}

	state.SetItemsProcessed(state.iterations());
}
} // namespace

BENCHMARK(BM_EmitNoSubscribers)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_EmitOneSubscriber)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_EmitManySubscribers)->RangeMultiplier(4)->Range(MinSubscribers, MaxSubscribers)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_SubscribeBurst)->RangeMultiplier(4)->Range(MinSubscribers, MaxSubscribers)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_SubscribeUnsubscribeChurn)->RangeMultiplier(4)->Range(MinSubscribers, MaxSubscribers)->Unit(benchmark::kNanosecond);
// Fixed iteration count: the off-clock drain runs `iterations x subscribers` handlers, so a 4.5 ns push
// would otherwise let min-time drive tens of millions of iterations into 10^11 calls of untimed work.
BENCHMARK(BM_QueueEmit)
	->RangeMultiplier(8)
	->Range(MinSubscribers, MaxSubscribers)
	->Iterations(100000)
	->Unit(benchmark::kNanosecond);

BENCHMARK(BM_EmitConcurrent)
	->Setup(SetupConcurrent)
	->Teardown(TeardownConcurrent)
	->Arg(8)
	->ThreadRange(1, 16)
	->Unit(benchmark::kNanosecond);

// Fixed iteration count: nothing drains mid-run, so the backlog must stay bounded.
BENCHMARK(BM_QueueEmitConcurrent)
	->Setup(SetupConcurrent)
	->Teardown(TeardownConcurrent)
	->Arg(8)
	->ThreadRange(1, 16)
	->Iterations(100000)
	->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
