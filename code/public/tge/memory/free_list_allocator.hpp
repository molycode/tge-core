#pragma once

#include "allocator.hpp"

namespace Tge::Memory
{

// General-purpose free-list allocator with first-fit strategy
class CFreeListAllocator final : public IAllocator
{
public:

	explicit CFreeListAllocator(size_t capacity);
	~CFreeListAllocator();

	void* Allocate(size_t size, size_t alignment) override;
	void Deallocate(void* ptr) override;

	std::string_view GetName() const override { return "FreeListAllocator"; }
	size_t GetTotalAllocated() const override { return m_totalAllocated.load(); }
	size_t GetCurrentUsage() const override { return m_currentUsage.load(); }
	size_t GetNumAllocations() const override { return m_numAllocations.load(); }

	size_t GetCapacity() const { return m_capacity; }

private:

	struct SAllocationHeader
	{
		size_t size;
		size_t padding;
	};

	struct SFreeBlock
	{
		size_t size;
		SFreeBlock* next;
	};

	void* m_buffer = nullptr;
	size_t m_capacity = 0;
	SFreeBlock* m_freeList = nullptr;

	void CoalesceFreeBlocks();
};

} // namespace Tge::Memory
