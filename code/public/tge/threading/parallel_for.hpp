#pragma once

#include "job_group.hpp"
#include <tge/assert.hpp>
#include <cstdint>

namespace Tge::Threading
{
uint32_t ComputeParallelChunkCount(uint32_t numItems, uint32_t grainSize);

// grainSize is not defaulted: fan-out costs ~50 us here, so the break-even is 50 us / the body's per-item cost.
// A cancelled enclosing group skips every chunk body, so this can return having silently done nothing.
template<typename Func>
void ParallelFor(uint32_t begin, uint32_t end, Func&& func, uint32_t grainSize)
{
	TGE_ASSERT(begin <= end, "ParallelFor called with an inverted range");

	uint32_t const numItems{ (end > begin) ? (end - begin) : 0u };
	uint32_t const numChunks{ ComputeParallelChunkCount(numItems, grainSize) };

	if (numChunks > 1u)
	{
		CJobGroup group;

		for (uint32_t chunk = 0u; chunk < numChunks; ++chunk)
		{
			// 64-bit product: exact partition, no empty trailing chunk, no overflow.
			uint64_t const span{ numItems };
			uint32_t const chunkBegin{ begin + static_cast<uint32_t>((span * chunk) / numChunks) };
			uint32_t const chunkEnd{ begin + static_cast<uint32_t>((span * (chunk + 1u)) / numChunks) };

			// By reference: Wait() joins before we return, so the capture cannot dangle.
			group.SubmitJob([&func, chunkBegin, chunkEnd]()
			{
				for (uint32_t index = chunkBegin; index < chunkEnd; ++index)
				{
					func(index);
				}
			});
		}

		// Wait() drains our own chunks inline, so a ParallelFor nested inside a job cannot deadlock.
		group.Wait();
	}
	else
	{
		for (uint32_t index = begin; index < end; ++index)
		{
			func(index);
		}
	}
}
} // namespace Tge::Threading
