#pragma once

#include "tracked_allocator.hpp"

namespace Tge::Memory
{
class CDefaultAllocator final : public CTrackedAllocator
{
public:

	CDefaultAllocator() = default;
	~CDefaultAllocator() = default;

	void* Allocate(size_t size, size_t alignment) override;
	void Deallocate(void* ptr) override;

	std::string_view GetName() const override { return "DefaultAllocator"; }
};

extern CDefaultAllocator g_defaultAllocator;
} // namespace Tge::Memory
