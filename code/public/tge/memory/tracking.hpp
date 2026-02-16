#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Tge::Memory
{
enum class ECategory : uint8_t
{
	// CPU Memory (global operators)
	Global,           // Uncategorized allocations through global new/delete
	STLContainers,    // std::vector/string/map with scoped category
	GameObjects,      // Mesh instances, entities
	AssetLoading,     // Model loading, texture decoding

	// GPU Memory
	VertexBuffers,
	IndexBuffers,
	UniformBuffers,
	StagingBuffers,
	Textures,
	RenderTargets,

	// System
	AssetCache,
	ObjectPools,
	DescriptorPools,
	CommandPools,
	Console,
	Debug,
	Other,

	Count
};

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

std::string_view GetCategoryName(ECategory category);

// Unified tracker - tracks all memory (global operators + explicit allocators + GPU)
void TrackAllocation(ECategory category, size_t bytes);
void TrackDeallocation(ECategory category, size_t bytes);

// Query functions
SStats GetStats(ECategory category);
size_t GetTotalAllocated();
size_t GetTotalDeallocated();
size_t GetCurrentUsage();
size_t GetNumAllocations();

// Utilities
void PrintStats();
void Reset();

} // namespace Tge::Memory
