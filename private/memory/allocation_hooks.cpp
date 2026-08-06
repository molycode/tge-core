#include <tge/memory/allocation_hooks.hpp>
#include <atomic>

namespace Tge::Memory
{
namespace
{
	std::atomic<AllocationHook> gOnAllocation    { nullptr };
	std::atomic<DeallocationHook> gOnDeallocation{ nullptr };
} // namespace

//////////////////////////////////////////////////////////////////////////
void SetAllocationHooks(AllocationHook onAlloc, DeallocationHook onFree)
{
	gOnAllocation.store(onAlloc, std::memory_order_relaxed);
	gOnDeallocation.store(onFree, std::memory_order_relaxed);
}

//////////////////////////////////////////////////////////////////////////
void NotifyAllocation(void* ptr, size_t size)
{
	AllocationHook const hook = gOnAllocation.load(std::memory_order_relaxed);

	if (hook != nullptr && ptr != nullptr)
	{
		hook(ptr, size);
	}
}

//////////////////////////////////////////////////////////////////////////
void NotifyDeallocation(void* ptr)
{
	DeallocationHook const hook = gOnDeallocation.load(std::memory_order_relaxed);

	if (hook != nullptr && ptr != nullptr)
	{
		hook(ptr);
	}
}
} // namespace Tge::Memory
