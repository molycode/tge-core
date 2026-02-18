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

	size_t GetTotalAllocated() const override { return m_totalAllocated.load(); }
	size_t GetCurrentUsage() const override   { return m_currentUsage.load(); }
	size_t GetNumAllocations() const override { return m_numAllocations.load(); }

protected:

	~CTrackedAllocator() = default;

	std::atomic<size_t> m_totalAllocated{0};
	std::atomic<size_t> m_currentUsage{0};
	std::atomic<size_t> m_numAllocations{0};
	std::atomic<size_t> m_numDeallocations{0};
};
} // namespace Tge::Memory
