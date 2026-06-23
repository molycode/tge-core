#pragma once

#include "allocator.hpp"
#include <limits>

namespace Tge::Memory
{

// STL-compatible allocator adapter
// Usage: std::vector<int, CStdAllocator<int>> myVec(CStdAllocator<int>(&gFrameAllocator));
template<typename T>
class CStdAllocator final
{
public:

	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using propagate_on_container_move_assignment = std::true_type;

	explicit CStdAllocator(IAllocator* allocator) noexcept
		: m_allocator(allocator)
	{
	}

	CStdAllocator(CStdAllocator const& other) noexcept
		: m_allocator(other.m_allocator)
	{
	}

	template<typename U>
	CStdAllocator(CStdAllocator<U> const& other) noexcept
		: m_allocator(other.m_allocator)
	{
	}

	~CStdAllocator() noexcept = default;

	CStdAllocator& operator=(CStdAllocator const&) noexcept = default;

	template<typename U>
	struct rebind
	{
		using other = CStdAllocator<U>;
	};

	[[nodiscard]] T* allocate(size_type n)
	{
		if (n > std::numeric_limits<size_type>::max() / sizeof(T))
		{
			throw std::bad_alloc();
		}

		return static_cast<T*>(m_allocator->Allocate(n * sizeof(T), alignof(T)));
	}

	void deallocate(T* ptr, size_type) noexcept
	{
		m_allocator->Deallocate(ptr);
	}

	template<typename U>
	bool operator==(CStdAllocator<U> const& other) const noexcept
	{
		return m_allocator == other.m_allocator;
	}

	template<typename U>
	bool operator!=(CStdAllocator<U> const& other) const noexcept
	{
		return m_allocator != other.m_allocator;
	}

	// Allow access to m_allocator for rebind constructor
	template<typename U>
	friend class CStdAllocator;

private:

	IAllocator* m_allocator;
};

} // namespace Tge::Memory
