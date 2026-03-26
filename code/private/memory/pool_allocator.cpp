#include <tge/memory/pool_allocator.hpp>

#include <cstdlib>
#include <algorithm>

namespace Tge::Memory
{

//////////////////////////////////////////////////////////////////////////
CPoolAllocator::CPoolAllocator(size_t objectSize, size_t objectCount)
	: m_objectSize(std::max(objectSize, sizeof(SFreeNode)))
	, m_objectCount(objectCount)
{
	if (m_objectCount > 0)
	{
		size_t const totalSize = m_objectSize * m_objectCount;
		m_buffer = std::malloc(totalSize);

		if (m_buffer != nullptr)
		{
			// Initialize free list - link all objects together
			uintptr_t bufferAddr = reinterpret_cast<uintptr_t>(m_buffer);

			for (size_t i = 0; i < m_objectCount; ++i)
			{
				SFreeNode* node = reinterpret_cast<SFreeNode*>(bufferAddr + i * m_objectSize);

				if (i < m_objectCount - 1)
				{
					node->next = reinterpret_cast<SFreeNode*>(bufferAddr + (i + 1) * m_objectSize);
				}
				else
				{
					node->next = nullptr;
				}
			}

			m_freeList = reinterpret_cast<SFreeNode*>(m_buffer);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CPoolAllocator::~CPoolAllocator()
{
	if (m_buffer != nullptr)
	{
		std::free(m_buffer);
		m_buffer = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void* CPoolAllocator::Allocate(size_t size, size_t)
{
	void* ptr = nullptr;

	if (size <= m_objectSize && m_freeList != nullptr)
	{
		ptr = m_freeList;
		m_freeList = m_freeList->next;

		RecordAlloc(m_objectSize);
	}

	return ptr;
}

//////////////////////////////////////////////////////////////////////////
void CPoolAllocator::Deallocate(void* ptr)
{
	if (ptr != nullptr)
	{
		// Return object to free list
		SFreeNode* node = static_cast<SFreeNode*>(ptr);
		node->next = m_freeList;
		m_freeList = node;

		RecordDealloc(m_objectSize);
	}
}

//////////////////////////////////////////////////////////////////////////
size_t CPoolAllocator::GetAvailableObjects() const
{
	size_t count = 0;
	SFreeNode* node = m_freeList;

	while (node != nullptr)
	{
		++count;
		node = node->next;
	}

	return count;
}

} // namespace Tge::Memory
