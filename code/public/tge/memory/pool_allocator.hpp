#pragma once

#include "allocator.hpp"
#include <vector>

namespace Tge::Memory
{

// Fixed-size object pool allocator
class CPoolAllocator final : public IAllocator
{
public:

	CPoolAllocator(size_t objectSize, size_t objectCount);
	~CPoolAllocator();

	void* Allocate(size_t size, size_t alignment) override;
	void Deallocate(void* ptr) override;

	std::string_view GetName() const override { return "PoolAllocator"; }
	size_t GetTotalAllocated() const override { return m_totalAllocated.load(); }
	size_t GetCurrentUsage() const override { return m_currentUsage.load(); }
	size_t GetNumAllocations() const override { return m_numAllocations.load(); }

	size_t GetObjectSize() const { return m_objectSize; }
	size_t GetObjectCount() const { return m_objectCount; }
	size_t GetAvailableObjects() const;

private:

	struct SFreeNode
	{
		SFreeNode* next;
	};

	void* m_buffer = nullptr;
	size_t m_objectSize = 0;
	size_t m_objectCount = 0;
	SFreeNode* m_freeList = nullptr;
};

} // namespace Tge::Memory
