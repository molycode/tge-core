#include <tge/memory/free_list_allocator.hpp>

#include <cstdlib>
#include <cstring>
#include <algorithm>

namespace Tge::Memory
{

//////////////////////////////////////////////////////////////////////////
CFreeListAllocator::CFreeListAllocator(size_t capacity)
	: m_capacity(capacity)
{
	if (capacity > 0)
	{
		m_buffer = std::malloc(capacity);

		if (m_buffer != nullptr)
		{
			// Initialize with single large free block
			m_freeList = static_cast<SFreeBlock*>(m_buffer);
			m_freeList->size = capacity;
			m_freeList->next = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CFreeListAllocator::~CFreeListAllocator()
{
	if (m_buffer != nullptr)
	{
		std::free(m_buffer);
		m_buffer = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void* CFreeListAllocator::Allocate(size_t size, size_t alignment)
{
	void* ptr = nullptr;

	if (size > 0 && m_freeList != nullptr)
	{
		size_t const headerSize = sizeof(SAllocationHeader);
		SFreeBlock* prevBlock = nullptr;
		SFreeBlock* currentBlock = m_freeList;

		// First-fit: find first block that fits
		while (currentBlock != nullptr)
		{
			uintptr_t const blockAddr = reinterpret_cast<uintptr_t>(currentBlock);
			uintptr_t const dataAddr = blockAddr + headerSize;
			uintptr_t const alignedAddr = (dataAddr + alignment - 1) & ~(alignment - 1);
			size_t const padding = alignedAddr - dataAddr;
			size_t const totalSize = headerSize + padding + size;

			if (currentBlock->size >= totalSize)
			{
				// Found suitable block
				size_t const remainingSize = currentBlock->size - totalSize;

				if (remainingSize > sizeof(SFreeBlock))
				{
					// Split block - create new free block with remaining space
					SFreeBlock* newBlock = reinterpret_cast<SFreeBlock*>(blockAddr + totalSize);
					newBlock->size = remainingSize;
					newBlock->next = currentBlock->next;

					if (prevBlock != nullptr)
					{
						prevBlock->next = newBlock;
					}
					else
					{
						m_freeList = newBlock;
					}
				}
				else
				{
					// Use entire block (remaining too small to split)
					if (prevBlock != nullptr)
					{
						prevBlock->next = currentBlock->next;
					}
					else
					{
						m_freeList = currentBlock->next;
					}
				}

				// Store allocation header immediately before aligned address
				SAllocationHeader* header = reinterpret_cast<SAllocationHeader*>(alignedAddr - headerSize);
				header->size = size;
				header->padding = padding;

				ptr = reinterpret_cast<void*>(alignedAddr);

				m_totalAllocated.fetch_add(size, std::memory_order_relaxed);
				m_currentUsage.fetch_add(size, std::memory_order_relaxed);
				m_numAllocations.fetch_add(1, std::memory_order_relaxed);

				break;
			}

			prevBlock = currentBlock;
			currentBlock = currentBlock->next;
		}
	}

	return ptr;
}

//////////////////////////////////////////////////////////////////////////
void CFreeListAllocator::Deallocate(void* ptr)
{
	if (ptr != nullptr && m_buffer != nullptr)
	{
		// Get header (stored immediately before aligned address)
		size_t const headerSize = sizeof(SAllocationHeader);
		uintptr_t const dataAddr = reinterpret_cast<uintptr_t>(ptr);
		SAllocationHeader const* header = reinterpret_cast<SAllocationHeader const*>(dataAddr - headerSize);

		size_t const totalSize = headerSize + header->padding + header->size;
		uintptr_t const blockAddr = dataAddr - headerSize - header->padding;

		// Create new free block
		SFreeBlock* newBlock = reinterpret_cast<SFreeBlock*>(blockAddr);
		newBlock->size = totalSize;
		newBlock->next = nullptr;

		// Insert into free list (sorted by address)
		SFreeBlock* prevBlock = nullptr;
		SFreeBlock* currentBlock = m_freeList;

		while (currentBlock != nullptr && reinterpret_cast<uintptr_t>(currentBlock) < blockAddr)
		{
			prevBlock = currentBlock;
			currentBlock = currentBlock->next;
		}

		if (prevBlock != nullptr)
		{
			prevBlock->next = newBlock;
		}
		else
		{
			m_freeList = newBlock;
		}

		newBlock->next = currentBlock;

		m_currentUsage.fetch_sub(header->size, std::memory_order_relaxed);
		m_numDeallocations.fetch_add(1, std::memory_order_relaxed);

		// Coalesce adjacent free blocks
		CoalesceFreeBlocks();
	}
}

//////////////////////////////////////////////////////////////////////////
void CFreeListAllocator::CoalesceFreeBlocks()
{
	SFreeBlock* currentBlock = m_freeList;

	while (currentBlock != nullptr && currentBlock->next != nullptr)
	{
		uintptr_t const currentEnd = reinterpret_cast<uintptr_t>(currentBlock) + currentBlock->size;
		uintptr_t const nextAddr = reinterpret_cast<uintptr_t>(currentBlock->next);

		if (currentEnd == nextAddr)
		{
			// Adjacent blocks - merge them
			currentBlock->size += currentBlock->next->size;
			currentBlock->next = currentBlock->next->next;
		}
		else
		{
			currentBlock = currentBlock->next;
		}
	}
}

} // namespace Tge::Memory
