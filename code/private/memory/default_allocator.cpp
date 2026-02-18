#include <tge/memory/default_allocator.hpp>
#include <tge/memory/tracking.hpp>

#include <rpmalloc/rpmalloc.h>

#include <iostream>

namespace Tge::Memory
{

CDefaultAllocator g_defaultAllocator;

CDefaultAllocator::CDefaultAllocator()
{
}

CDefaultAllocator::~CDefaultAllocator()
{
}

void* CDefaultAllocator::Allocate(size_t size, size_t alignment)
{
	void* ptr = nullptr;

	if (size > 0)
	{
		if (alignment <= sizeof(void*))
		{
			// Default alignment - use regular rpmalloc
			ptr = rpmalloc(size);
		}
		else
		{
			// Custom alignment - use aligned allocation
			ptr = rpaligned_alloc(alignment, size);
		}

		if (ptr != nullptr)
		{
			size_t const usableSize = rpmalloc_usable_size(ptr);

			m_totalAllocated.fetch_add(usableSize, std::memory_order_relaxed);
			m_currentUsage.fetch_add(usableSize, std::memory_order_relaxed);
			m_numAllocations.fetch_add(1, std::memory_order_relaxed);

#ifdef TGE_MEMORY_TRACKING_ENABLED
			TrackAllocation(Category::Other, usableSize);
#endif // TGE_MEMORY_TRACKING_ENABLED
		}
	}

	return ptr;
}

void CDefaultAllocator::Deallocate(void* ptr)
{
	if (ptr != nullptr)
	{
		size_t const usableSize = rpmalloc_usable_size(ptr);

		rpfree(ptr);

		m_currentUsage.fetch_sub(usableSize, std::memory_order_relaxed);
		m_numDeallocations.fetch_add(1, std::memory_order_relaxed);

#ifdef TGE_MEMORY_TRACKING_ENABLED
		TrackDeallocation(Category::Other, usableSize);
#endif // TGE_MEMORY_TRACKING_ENABLED
	}
}

} // namespace Tge::Memory
