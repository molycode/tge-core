#pragma once

#include "tracked_allocator.hpp"

namespace Tge::Memory
{

// Stack/LIFO allocator - deallocations must occur in reverse order
class CStackAllocator final : public CTrackedAllocator
{
public:

	explicit CStackAllocator(size_t capacity);
	~CStackAllocator();

	void* Allocate(size_t size, size_t alignment) override;
	void Deallocate(void* ptr) override;

	std::string_view GetName() const override { return "StackAllocator"; }

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
