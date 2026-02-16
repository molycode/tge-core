#pragma once

#include "allocator.hpp"

namespace Tge::Memory
{

// Stack/LIFO allocator - deallocations must occur in reverse order
class CStackAllocator final : public IAllocator
{
public:

	explicit CStackAllocator(size_t capacity);
	~CStackAllocator();

	void* Allocate(size_t size, size_t alignment) override;
	void Deallocate(void* ptr) override;

	std::string_view GetName() const override { return "StackAllocator"; }
	size_t GetTotalAllocated() const override { return m_totalAllocated.load(); }
	size_t GetCurrentUsage() const override { return m_currentUsage.load(); }
	size_t GetNumAllocations() const override { return m_numAllocations.load(); }

	void Reset();
	size_t GetCapacity() const { return m_capacity; }

private:

	struct SAllocationHeader
	{
		size_t size;
		size_t padding;
	};

	void* m_buffer = nullptr;
	size_t m_capacity = 0;
	size_t m_offset = 0;
	void* m_lastAllocation = nullptr;
};

} // namespace Tge::Memory
