#include <tge/memory/linear_allocator.hpp>
#include <tge/memory/tracking.hpp>

#include <cstdlib>
#include <cstring>

namespace Tge::Memory
{

CLinearAllocator g_frameAllocator(16 * 1024 * 1024);

CLinearAllocator::CLinearAllocator(size_t capacity)
	: m_capacity(capacity)
{
	if (capacity > 0)
	{
		m_buffer = std::malloc(capacity);
	}
}

CLinearAllocator::~CLinearAllocator()
{
	if (m_buffer != nullptr)
	{
		std::free(m_buffer);
		m_buffer = nullptr;
	}
}

void* CLinearAllocator::Allocate(size_t size, size_t alignment)
{
	void* ptr = nullptr;

	if (size > 0 && m_buffer != nullptr)
	{
		// Calculate aligned offset
		uintptr_t const currentAddr = reinterpret_cast<uintptr_t>(m_buffer) + m_offset;
		uintptr_t const alignedAddr = (currentAddr + alignment - 1) & ~(alignment - 1);
		size_t const padding = alignedAddr - currentAddr;
		size_t const totalSize = padding + size;

		if (m_offset + totalSize <= m_capacity)
		{
			ptr = reinterpret_cast<void*>(alignedAddr);
			m_offset += totalSize;

			m_totalAllocated.fetch_add(size, std::memory_order_relaxed);
			m_currentUsage.fetch_add(size, std::memory_order_relaxed);
			m_numAllocations.fetch_add(1, std::memory_order_relaxed);

#ifdef TGE_MEMORY_TRACKING_ENABLED
			TrackAllocation(Category::Other, size);
#endif // TGE_MEMORY_TRACKING_ENABLED
		}
	}

	return ptr;
}

void CLinearAllocator::Deallocate([[maybe_unused]] void* ptr)
{
	// Linear allocator doesn't support individual deallocations
	// Memory is only freed on Reset()
}

void CLinearAllocator::Reset()
{
#ifdef TGE_MEMORY_TRACKING_ENABLED
	size_t const currentUsage = m_currentUsage.load(std::memory_order_relaxed);

	if (currentUsage > 0)
	{
		TrackDeallocation(Category::Other, currentUsage);
	}
#endif // TGE_MEMORY_TRACKING_ENABLED

	m_offset = 0;
	m_currentUsage.store(0, std::memory_order_relaxed);
}

} // namespace Tge::Memory
