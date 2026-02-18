#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Tge::Memory
{
using CategoryId = uint16_t;

namespace Category
{
	constexpr CategoryId Global       = 0;
	constexpr CategoryId Other        = 1;
	constexpr CategoryId BuiltInCount = 2;
} // namespace Category

struct SStats final
{
	size_t allocated = 0;
	size_t deallocated = 0;
	size_t peakUsage = 0;
	uint32_t allocationCount = 0;
	uint32_t deallocationCount = 0;

	[[nodiscard]] size_t GetCurrentUsage() const
	{
		return allocated - deallocated;
	}
};

// Register a project-specific category. Name must have static lifetime (use string literals).
// Returns a CategoryId for use with TrackAllocation/TrackDeallocation.
CategoryId RegisterCategory(std::string_view name);

std::string_view GetCategoryName(CategoryId category);

// Unified tracker - tracks all memory (global operators + explicit allocators)
void TrackAllocation(CategoryId category, size_t bytes);
void TrackDeallocation(CategoryId category, size_t bytes);

// Query functions
SStats GetStats(CategoryId category);
size_t GetTotalAllocated();
size_t GetTotalDeallocated();
size_t GetCurrentUsage();
size_t GetNumAllocations();

// Utilities
void PrintStats();
void Reset();

} // namespace Tge::Memory
