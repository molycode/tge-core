#include "../loggers.hpp"
#include <tge/memory/tracking.hpp>
#include <array>
#include <mutex>

#ifndef TGE_LOGGING_ENABLED
#include <format>
#endif // TGE_LOGGING_ENABLED

namespace Tge::Memory
{
namespace
{
	std::mutex g_trackingMutex;
	std::array<SStats, static_cast<size_t>(ECategory::Count)> g_categoryStats = {};
}

//////////////////////////////////////////////////////////////////////////
void TrackAllocation(ECategory category, size_t bytes)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t const catIndex = static_cast<size_t>(category);
	g_categoryStats[catIndex].allocated += bytes;
	g_categoryStats[catIndex].allocationCount++;

	size_t const currentUsage = g_categoryStats[catIndex].GetCurrentUsage();

	if (currentUsage > g_categoryStats[catIndex].peakUsage)
	{
		g_categoryStats[catIndex].peakUsage = currentUsage;
	}
}

//////////////////////////////////////////////////////////////////////////
void TrackDeallocation(ECategory category, size_t bytes)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t const catIndex = static_cast<size_t>(category);
	g_categoryStats[catIndex].deallocated += bytes;
	g_categoryStats[catIndex].deallocationCount++;
}

//////////////////////////////////////////////////////////////////////////
SStats GetStats(ECategory category)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);
	return g_categoryStats[static_cast<size_t>(category)];
}

//////////////////////////////////////////////////////////////////////////
size_t GetTotalAllocated()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (auto const& stat : g_categoryStats)
	{
		total += stat.allocated;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetTotalDeallocated()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (auto const& stat : g_categoryStats)
	{
		total += stat.deallocated;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetCurrentUsage()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (auto const& stat : g_categoryStats)
	{
		total += stat.GetCurrentUsage();
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetNumAllocations()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (auto const& stat : g_categoryStats)
	{
		total += stat.allocationCount;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
std::string_view GetCategoryName(ECategory category)
{
	switch (category)
	{
		case ECategory::Global:           return "Global";
		case ECategory::STLContainers:    return "STL Containers";
		case ECategory::GameObjects:      return "Game Objects";
		case ECategory::AssetLoading:     return "Asset Loading";
		case ECategory::VertexBuffers:    return "Vertex Buffers";
		case ECategory::IndexBuffers:     return "Index Buffers";
		case ECategory::UniformBuffers:   return "Uniform Buffers";
		case ECategory::StagingBuffers:   return "Staging Buffers";
		case ECategory::Textures:         return "Textures";
		case ECategory::RenderTargets:    return "Render Targets";
		case ECategory::AssetCache:       return "Asset Cache";
		case ECategory::ObjectPools:      return "Object Pools";
		case ECategory::DescriptorPools:  return "Descriptor Pools";
		case ECategory::CommandPools:     return "Command Pools";
		case ECategory::Console:          return "Console";
		case ECategory::Debug:            return "Debug";
		case ECategory::Other:            return "Other";
		case ECategory::Count:            return "Invalid";
	}

	return "Unknown";
}

//////////////////////////////////////////////////////////////////////////
void PrintStats()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	gLog.Info("");
	gLog.Info("=== TgeCore Unified Memory Statistics ===");

	constexpr float MiB = 1024.0f * 1024.0f;
	constexpr float KiB = 1024.0f;

	// Print per-category breakdown (only categories with allocations)
	for (size_t i = 0; i < static_cast<size_t>(ECategory::Count); ++i)
	{
		auto const& stat = g_categoryStats[i];

		if (stat.allocated > 0)
		{
			ECategory cat = static_cast<ECategory>(i);
			float const currentUsage = stat.GetCurrentUsage();

			std::string sizeStr;

			if (currentUsage < MiB)
			{
				sizeStr = std::format("{:.2f} KiB", currentUsage / KiB);
			}
			else
			{
				sizeStr = std::format("{:.2f} MiB", currentUsage / MiB);
			}

			gLog.Info("  {:<20}: {} ({} allocs)",
				GetCategoryName(cat), sizeStr, stat.allocationCount);
		}
	}

	// Print totals
	size_t totalCurrent = 0;
	size_t totalAllocated = 0;
	size_t totalAllocations = 0;

	for (auto const& stat : g_categoryStats)
	{
		totalCurrent += stat.GetCurrentUsage();
		totalAllocated += stat.allocated;
		totalAllocations += stat.allocationCount;
	}

	gLog.Info("");
	gLog.Info("Total:");

	if (totalCurrent < MiB)
	{
		gLog.Info("  Current Usage:   {:.2f} KiB", totalCurrent / KiB);
		gLog.Info("  Total Allocated: {:.2f} KiB", totalAllocated / KiB);
	}
	else
	{
		gLog.Info("  Current Usage:   {:.2f} MiB", totalCurrent / MiB);
		gLog.Info("  Total Allocated: {:.2f} MiB", totalAllocated / MiB);
	}

	gLog.Info("  Allocations:     {}", totalAllocations);
	gLog.Info("=========================================");
	gLog.Info("");
}

//////////////////////////////////////////////////////////////////////////
void Reset()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);
	g_categoryStats = {};
}
} // namespace Tge::Memory
