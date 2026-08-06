#pragma once

#include <cstddef>
#include <string_view>

namespace Tge::Memory
{
class IAllocator
{
public:

	virtual void* Allocate(size_t size, size_t alignment) = 0;
	virtual void Deallocate(void* ptr) = 0;

	virtual std::string_view GetName() const = 0;
	virtual size_t GetTotalAllocated() const = 0;
	virtual size_t GetCurrentUsage() const = 0;
	virtual size_t GetNumAllocations() const = 0;
	virtual size_t GetNumDeallocations() const = 0;

protected:

	~IAllocator() = default;
};
} // namespace Tge::Memory
