#include <tge/threading/parallel_for.hpp>
#include <tge/threading/job_system.hpp>
#include <tge/logging/loggers.hpp>
#include <algorithm>

namespace Tge::Threading
{
//////////////////////////////////////////////////////////////////////////
uint32_t ComputeParallelChunkCount(uint32_t numItems, uint32_t grainSize)
{
	uint32_t     numChunks{ 0u };
	size_t const numThreads{ GetNumThreads() };

	if (numThreads > 0u)
	{
		uint32_t const effectiveGrain{ (grainSize > 0u) ? grainSize : 1u };
		uint32_t const chunkLimit{ (numItems + effectiveGrain - 1u) / effectiveGrain };

		// One chunk per worker: an extra chunk costs ~0.85 us to submit, more than the rebalancing it buys.
		numChunks = std::min(static_cast<uint32_t>(numThreads), chunkLimit);
	}
	else
	{
		gLog.Warning("ParallelFor ran serially: the job system is not initialized");
	}

	return numChunks;
}
} // namespace Tge::Threading
