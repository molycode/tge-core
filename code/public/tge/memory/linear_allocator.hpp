#pragma once

#include "allocator.hpp"

namespace Tge::Memory
{
// Fast bump allocator - allocates linearly, resets all at once (perfect for per-frame temp allocations)
class CLinearAllocator final : public IAllocator
{
public:

	explicit CLinearAllocator(size_t capacity);
	~CLinearAllocator();

	void* Allocate(size_t size, size_t alignment) override;
	void Deallocate(void* ptr) override; // No-op

	std::string_view GetName() const override { return "LinearAllocator"; }
	size_t GetTotalAllocated() const override { return m_totalAllocated.load(); }
	size_t GetCurrentUsage() const override { return m_currentUsage.load(); }
	size_t GetNumAllocations() const override { return m_numAllocations.load(); }

	void Reset();
	size_t GetCapacity() const { return m_capacity; }

private:

	void* m_buffer = nullptr;
	size_t m_capacity = 0;
	size_t m_offset = 0;
};

extern CLinearAllocator g_frameAllocator;
} // namespace Tge::Memory
