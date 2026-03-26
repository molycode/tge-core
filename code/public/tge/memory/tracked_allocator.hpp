#pragma once

#include "allocator.hpp"
#include <atomic>

namespace Tge::Memory
{
// Intermediate base providing atomic tracking state shared by all concrete allocators.
// Implements GetTotalAllocated, GetCurrentUsage, and GetNumAllocations so concrete
// allocators only need to update the atomics, not re-declare the getters.
class CTrackedAllocator : public IAllocator
{
public:

	size_t GetTotalAllocated() const override    { return m_totalAllocated.load(); }
	size_t GetCurrentUsage() const override      { return m_currentUsage.load(); }
	size_t GetNumAllocations() const override    { return m_numAllocations.load(); }
	size_t GetNumDeallocations() const override  { return m_numDeallocations.load(); }

protected:

	~CTrackedAllocator() = default;

	void RecordAlloc(size_t bytes)
	{
		m_totalAllocated.fetch_add(bytes, std::memory_order_relaxed);
		m_currentUsage.fetch_add(bytes, std::memory_order_relaxed);
		m_numAllocations.fetch_add(1, std::memory_order_relaxed);
	}

	void RecordDealloc(size_t bytes)
	{
		m_currentUsage.fetch_sub(bytes, std::memory_order_relaxed);
		m_numDeallocations.fetch_add(1, std::memory_order_relaxed);
	}

	std::atomic<size_t> m_totalAllocated{0};
	std::atomic<size_t> m_currentUsage{0};
	std::atomic<size_t> m_numAllocations{0};
	std::atomic<size_t> m_numDeallocations{0};
};
} // namespace Tge::Memory
