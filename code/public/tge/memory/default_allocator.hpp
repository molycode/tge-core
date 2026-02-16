#pragma once

#include "allocator.hpp"

namespace Tge::Memory
{
class CDefaultAllocator final : public IAllocator
{
public:

	CDefaultAllocator();
	~CDefaultAllocator();

	void* Allocate(size_t size, size_t alignment) override;
	void Deallocate(void* ptr) override;

	std::string_view GetName() const override { return "DefaultAllocator"; }
	size_t GetTotalAllocated() const override { return m_totalAllocated.load(); }
	size_t GetCurrentUsage() const override { return m_currentUsage.load(); }
	size_t GetNumAllocations() const override { return m_numAllocations.load(); }
};

extern CDefaultAllocator g_defaultAllocator;
} // namespace Tge::Memory
