#include <tge/memory/stack_allocator.hpp>

#include <cstdlib>
#include <cstring>

namespace Tge::Memory
{

//////////////////////////////////////////////////////////////////////////
CStackAllocator::CStackAllocator(size_t capacity)
	: m_capacity(capacity)
{
	if (capacity > 0)
	{
		m_buffer = std::malloc(capacity);
	}
}

//////////////////////////////////////////////////////////////////////////
CStackAllocator::~CStackAllocator()
{
	if (m_buffer != nullptr)
	{
		std::free(m_buffer);
		m_buffer = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void* CStackAllocator::Allocate(size_t size, size_t alignment)
{
	void* ptr = nullptr;

	if (size > 0 && m_buffer != nullptr)
	{
		// Calculate aligned offset
		uintptr_t const currentAddr = reinterpret_cast<uintptr_t>(m_buffer) + m_offset;
		size_t const headerSize = sizeof(SAllocationHeader);
		uintptr_t const dataAddr = currentAddr + headerSize;
		uintptr_t const alignedAddr = (dataAddr + alignment - 1) & ~(alignment - 1);
		size_t const padding = alignedAddr - dataAddr;
		size_t const totalSize = headerSize + padding + size;

		if (m_offset + totalSize <= m_capacity)
		{
			// Store allocation header immediately before aligned address
			SAllocationHeader* header = reinterpret_cast<SAllocationHeader*>(alignedAddr - headerSize);
			header->size = size;
			header->padding = padding;

			ptr = reinterpret_cast<void*>(alignedAddr);
			m_lastAllocation = ptr;
			m_offset += totalSize;

			RecordAlloc(size);
		}
	}

	return ptr;
}

//////////////////////////////////////////////////////////////////////////
void CStackAllocator::Deallocate(void* ptr)
{
	if (ptr == m_lastAllocation && ptr != nullptr)
	{
		// Get header (stored immediately before aligned address)
		uintptr_t const dataAddr = reinterpret_cast<uintptr_t>(ptr);
		size_t const headerSize = sizeof(SAllocationHeader);
		SAllocationHeader* header = reinterpret_cast<SAllocationHeader*>(dataAddr - headerSize);

		size_t const totalSize = headerSize + header->padding + header->size;
		m_offset -= totalSize;

		RecordDealloc(header->size);

		// Update last allocation pointer (walk backwards to find previous)
		m_lastAllocation = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CStackAllocator::Reset()
{
	m_offset = 0;
	m_lastAllocation = nullptr;
	m_currentUsage.store(0, std::memory_order_relaxed);
}

} // namespace Tge::Memory
