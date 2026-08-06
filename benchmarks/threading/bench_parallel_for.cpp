#include <benchmark/benchmark.h>
#include <tge/init/init.hpp>
#include <tge/threading/parallel_for.hpp>
#include <tge/threading/job_system.hpp>
#include <cstdint>
#include <vector>

using namespace Tge::Threading;

namespace
{
// An LCG step is dependent, so it cannot be hoisted or vectorised away: `iterations` buys a real per-item cost.
uint64_t BusyWork(uint32_t index, uint32_t iterations)
{
	uint64_t acc{ index };

	for (uint32_t step = 0u; step < iterations; ++step)
	{
		acc = (acc * 6364136223846793005ull) + 1442695040888963407ull;
	}

	return acc;
}

//////////////////////////////////////////////////////////////////////////
void BM_SerialFor(benchmark::State& state)
{
	uint32_t const        numItems{ static_cast<uint32_t>(state.range(0)) };
	uint32_t const        iterations{ static_cast<uint32_t>(state.range(1)) };
	std::vector<uint64_t> out(numItems, 0u);

	for (auto _ : state)
	{
		for (uint32_t index = 0u; index < numItems; ++index)
		{
			out[index] = BusyWork(index, iterations);
		}

		benchmark::DoNotOptimize(out.data());
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * numItems);
}

//////////////////////////////////////////////////////////////////////////
void BM_ParallelFor(benchmark::State& state)
{
	uint32_t const        numItems{ static_cast<uint32_t>(state.range(0)) };
	uint32_t const        iterations{ static_cast<uint32_t>(state.range(1)) };
	std::vector<uint64_t> out(numItems, 0u);

	// Grain 1 keeps even tiny ranges on the parallel path.
	for (auto _ : state)
	{
		ParallelFor(0u, numItems, [&out, iterations](uint32_t index)
		{
			out[index] = BusyWork(index, iterations);
		}, 1u);

		benchmark::DoNotOptimize(out.data());
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * numItems);
}

//////////////////////////////////////////////////////////////////////////
void BM_ParallelForEmptyBody(benchmark::State& state)
{
	uint32_t const numItems{ static_cast<uint32_t>(state.range(0)) };

	// The fixed cost of one ParallelFor, with no body to hide it.
	for (auto _ : state)
	{
		ParallelFor(0u, numItems, [](uint32_t index)
		{
			benchmark::DoNotOptimize(index);
		}, 1u);
	}
}

//////////////////////////////////////////////////////////////////////////
// Cost grows with the index, so a static split leaves the last chunk holding the bag.
void BM_ParallelForImbalanced(benchmark::State& state)
{
	uint32_t const        numItems{ static_cast<uint32_t>(state.range(0)) };
	uint32_t const        iterations{ static_cast<uint32_t>(state.range(1)) };
	std::vector<uint64_t> out(numItems, 0u);

	for (auto _ : state)
	{
		ParallelFor(0u, numItems, [&out, iterations, numItems](uint32_t index)
		{
			out[index] = BusyWork(index, (iterations * index) / numItems);
		}, 1u);

		benchmark::DoNotOptimize(out.data());
		benchmark::ClobberMemory();
	}

	state.SetItemsProcessed(state.iterations() * numItems);
}

void CostSweep(benchmark::Benchmark* bench)
{
	for (int64_t const numItems : { 64, 256, 512, 1024, 4096 })
	{
		for (int64_t const iterations : { 0, 8, 64, 512 })
		{
			bench->Args({ numItems, iterations });
		}
	}
}
} // namespace

BENCHMARK(BM_SerialFor)->Apply(CostSweep)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ParallelFor)->Apply(CostSweep)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ParallelForImbalanced)->Apply(CostSweep)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_ParallelForEmptyBody)->RangeMultiplier(4)->Range(64, 4096)->Unit(benchmark::kMicrosecond);

//////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
	// ParallelFor falls back to serial without the job system.
	Tge::Initialize();

	::benchmark::Initialize(&argc, argv);
	::benchmark::RunSpecifiedBenchmarks();
	::benchmark::Shutdown();

	Tge::Terminate();

	return 0;
}
