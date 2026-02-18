#include "../logging/loggers.hpp"
#include <tge/memory/tracking.hpp>
#include <array>
#include <cassert>
#include <mutex>

#ifndef TGE_LOGGING_ENABLED
#include <format>
#endif // TGE_LOGGING_ENABLED

namespace Tge::Memory
{
namespace
{
	constexpr size_t MaxCategories = 256;

	std::mutex g_trackingMutex;
	std::array<SStats, MaxCategories> g_categoryStats = {};
	std::array<std::string_view, MaxCategories> g_categoryNames = {};
	size_t g_numCategories = Category::BuiltInCount;

	struct SBuiltInInit
	{
		SBuiltInInit()
		{
			g_categoryNames[Category::Global] = "Global";
			g_categoryNames[Category::Other]  = "Other";
		}
	} g_builtInInit;
} // namespace

//////////////////////////////////////////////////////////////////////////
CategoryId RegisterCategory(std::string_view name)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	assert(g_numCategories < MaxCategories && "RegisterCategory: maximum category limit reached");

	CategoryId const id = static_cast<CategoryId>(g_numCategories);
	g_categoryNames[g_numCategories] = name;
	g_numCategories++;
	return id;
}

//////////////////////////////////////////////////////////////////////////
size_t GetNumCategories()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);
	return g_numCategories;
}

//////////////////////////////////////////////////////////////////////////
void TrackAllocation(CategoryId category, size_t bytes)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	g_categoryStats[category].allocated += bytes;
	g_categoryStats[category].allocationCount++;

	size_t const currentUsage = g_categoryStats[category].GetCurrentUsage();

	if (currentUsage > g_categoryStats[category].peakUsage)
	{
		g_categoryStats[category].peakUsage = currentUsage;
	}
}

//////////////////////////////////////////////////////////////////////////
void TrackDeallocation(CategoryId category, size_t bytes)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	g_categoryStats[category].deallocated += bytes;
	g_categoryStats[category].deallocationCount++;
}

//////////////////////////////////////////////////////////////////////////
SStats GetStats(CategoryId category)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);
	return g_categoryStats[category];
}

//////////////////////////////////////////////////////////////////////////
size_t GetTotalAllocated()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < g_numCategories; ++i)
	{
		total += g_categoryStats[i].allocated;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetTotalDeallocated()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < g_numCategories; ++i)
	{
		total += g_categoryStats[i].deallocated;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetCurrentUsage()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < g_numCategories; ++i)
	{
		total += g_categoryStats[i].GetCurrentUsage();
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetNumAllocations()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < g_numCategories; ++i)
	{
		total += g_categoryStats[i].allocationCount;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
std::string_view GetCategoryName(CategoryId category)
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);
	return g_categoryNames[category];
}

//////////////////////////////////////////////////////////////////////////
void PrintStats()
{
	std::lock_guard<std::mutex> lock(g_trackingMutex);

	gLog.Info("");
	gLog.Info("=== TgeCore Unified Memory Statistics ===");

	constexpr float MiB = 1024.0f * 1024.0f;
	constexpr float KiB = 1024.0f;

	for (size_t i = 0; i < g_numCategories; ++i)
	{
		auto const& stat = g_categoryStats[i];

		if (stat.allocated > 0)
		{
			float const currentUsage = static_cast<float>(stat.GetCurrentUsage());

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
				g_categoryNames[i], sizeStr, stat.allocationCount);
		}
	}

	size_t totalCurrent = 0;
	size_t totalAllocated = 0;
	size_t totalAllocations = 0;

	for (size_t i = 0; i < g_numCategories; ++i)
	{
		totalCurrent += g_categoryStats[i].GetCurrentUsage();
		totalAllocated += g_categoryStats[i].allocated;
		totalAllocations += g_categoryStats[i].allocationCount;
	}

	gLog.Info("");
	gLog.Info("Total:");

	if (totalCurrent < MiB)
	{
		gLog.Info("  Current Usage:   {:.2f} KiB", static_cast<float>(totalCurrent) / KiB);
		gLog.Info("  Total Allocated: {:.2f} KiB", static_cast<float>(totalAllocated) / KiB);
	}
	else
	{
		gLog.Info("  Current Usage:   {:.2f} MiB", static_cast<float>(totalCurrent) / MiB);
		gLog.Info("  Total Allocated: {:.2f} MiB", static_cast<float>(totalAllocated) / MiB);
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
