#include <tge/memory/tracking.hpp>
#include <tge/logging/loggers.hpp>
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

	std::mutex gTrackingMutex;
	std::array<SStats, MaxCategories> gCategoryStats = {};
	std::array<std::string_view, MaxCategories> gCategoryNames = {};
	size_t gNumCategories = Category::BuiltInCount;

	struct SBuiltInInit
	{
		SBuiltInInit()
		{
			gCategoryNames[Category::Global] = "Global";
			gCategoryNames[Category::Other]  = "Other";
		}
	} gBuiltInInit;
} // namespace

//////////////////////////////////////////////////////////////////////////
CategoryId RegisterCategory(std::string_view name)
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	assert(gNumCategories < MaxCategories && "RegisterCategory: maximum category limit reached");

	CategoryId const id = static_cast<CategoryId>(gNumCategories);
	gCategoryNames[gNumCategories] = name;
	gNumCategories++;
	return id;
}

//////////////////////////////////////////////////////////////////////////
size_t GetNumCategories()
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);
	return gNumCategories;
}

//////////////////////////////////////////////////////////////////////////
void TrackAllocation(CategoryId category, size_t bytes)
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	gCategoryStats[category].allocated += bytes;
	gCategoryStats[category].allocationCount++;

	size_t const currentUsage = gCategoryStats[category].GetCurrentUsage();

	if (currentUsage > gCategoryStats[category].peakUsage)
	{
		gCategoryStats[category].peakUsage = currentUsage;
	}
}

//////////////////////////////////////////////////////////////////////////
void TrackDeallocation(CategoryId category, size_t bytes)
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	gCategoryStats[category].deallocated += bytes;
	gCategoryStats[category].deallocationCount++;
}

//////////////////////////////////////////////////////////////////////////
SStats GetStats(CategoryId category)
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);
	return gCategoryStats[category];
}

//////////////////////////////////////////////////////////////////////////
size_t GetTotalAllocated()
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < gNumCategories; ++i)
	{
		total += gCategoryStats[i].allocated;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetTotalDeallocated()
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < gNumCategories; ++i)
	{
		total += gCategoryStats[i].deallocated;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetCurrentUsage()
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < gNumCategories; ++i)
	{
		total += gCategoryStats[i].GetCurrentUsage();
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
size_t GetNumAllocations()
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	size_t total = 0;

	for (size_t i = 0; i < gNumCategories; ++i)
	{
		total += gCategoryStats[i].allocationCount;
	}

	return total;
}

//////////////////////////////////////////////////////////////////////////
std::string_view GetCategoryName(CategoryId category)
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);
	return gCategoryNames[category];
}

//////////////////////////////////////////////////////////////////////////
void PrintStats()
{
	std::lock_guard<std::mutex> lock(gTrackingMutex);

	gLog.Info("");
	gLog.Info("=== TgeCore Unified Memory Statistics ===");

	constexpr float MiB = 1024.0f * 1024.0f;
	constexpr float KiB = 1024.0f;

	for (size_t i = 0; i < gNumCategories; ++i)
	{
		auto const& stat = gCategoryStats[i];

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
				gCategoryNames[i], sizeStr, stat.allocationCount);
		}
	}

	size_t totalCurrent = 0;
	size_t totalAllocated = 0;
	size_t totalAllocations = 0;

	for (size_t i = 0; i < gNumCategories; ++i)
	{
		totalCurrent += gCategoryStats[i].GetCurrentUsage();
		totalAllocated += gCategoryStats[i].allocated;
		totalAllocations += gCategoryStats[i].allocationCount;
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
	std::lock_guard<std::mutex> lock(gTrackingMutex);
	gCategoryStats = {};
}
} // namespace Tge::Memory
