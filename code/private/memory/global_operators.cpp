#include <tge/memory/tracking.hpp>
#include <tge/memory/category_context.hpp>
#include <rpmalloc/rpmalloc.h>
#include <new>
#include <cstdint>

#ifdef TGE_MEMORY_TRACKING_ENABLED

namespace
{
	//////////////////////////////////////////////////////////////////////////
	// Allocation header stored before each allocation to track category
	struct SAllocationHeader
	{
		Tge::Memory::CategoryId category;  // 2 bytes (uint16_t)
		uint8_t padding[6];                // 6 bytes padding (align size_t)
		size_t size;                       // 8 bytes (requested size, offset 8)
	};
	static_assert(sizeof(SAllocationHeader) == 16);

	//////////////////////////////////////////////////////////////////////////
	// Get header pointer from user pointer
	SAllocationHeader* GetHeader(void* userPtr)
	{
		return reinterpret_cast<SAllocationHeader*>(
			static_cast<char*>(userPtr) - sizeof(SAllocationHeader)
		);
	}

	//////////////////////////////////////////////////////////////////////////
	// Get user pointer from raw allocation
	void* GetUserPtr(void* rawPtr)
	{
		return static_cast<char*>(rawPtr) + sizeof(SAllocationHeader);
	}

	//////////////////////////////////////////////////////////////////////////
	// Get header size aligned to specified alignment
	size_t GetAlignedHeaderSize(size_t alignment)
	{
		size_t const headerSize = sizeof(SAllocationHeader);
		return (headerSize + alignment - 1) & ~(alignment - 1);
	}

} // namespace

#endif // TGE_MEMORY_TRACKING_ENABLED

//////////////////////////////////////////////////////////////////////////
void* operator new(size_t size)
{
#ifdef TGE_MEMORY_TRACKING_ENABLED
	size_t const totalSize = sizeof(SAllocationHeader) + size;
	void* rawPtr = rpmalloc(totalSize);

	auto* header = static_cast<SAllocationHeader*>(rawPtr);
	header->category = Tge::Memory::GetCurrentCategory();
	header->size = size;

	void* userPtr = GetUserPtr(rawPtr);
	Tge::Memory::TrackAllocation(header->category, size);
	return userPtr;
#else
	return rpmalloc(size);
#endif // TGE_MEMORY_TRACKING_ENABLED
}

//////////////////////////////////////////////////////////////////////////
void operator delete(void* ptr) noexcept
{
	if (ptr != nullptr)
	{
#ifdef TGE_MEMORY_TRACKING_ENABLED
		SAllocationHeader* header = GetHeader(ptr);
		void* rawPtr = static_cast<void*>(header);

		Tge::Memory::TrackDeallocation(header->category, header->size);

		rpfree(rawPtr);
#else
		rpfree(ptr);
#endif // TGE_MEMORY_TRACKING_ENABLED
	}
}

//////////////////////////////////////////////////////////////////////////
void* operator new[](size_t size)
{
	return ::operator new(size);
}

//////////////////////////////////////////////////////////////////////////
void operator delete[](void* ptr) noexcept
{
	::operator delete(ptr);
}

//////////////////////////////////////////////////////////////////////////
void operator delete(void* ptr, size_t) noexcept
{
	::operator delete(ptr);
}

//////////////////////////////////////////////////////////////////////////
void operator delete[](void* ptr, size_t) noexcept
{
	::operator delete(ptr);
}

//////////////////////////////////////////////////////////////////////////
void* operator new(size_t size, std::align_val_t align)
{
#ifdef TGE_MEMORY_TRACKING_ENABLED
	size_t const alignment = static_cast<size_t>(align);
	size_t const headerSize = GetAlignedHeaderSize(alignment);
	size_t const totalSize = headerSize + size;

	void* rawPtr = rpaligned_alloc(alignment, totalSize);

	auto* header = static_cast<SAllocationHeader*>(rawPtr);
	header->category = Tge::Memory::GetCurrentCategory();
	header->size = size;

	void* userPtr = static_cast<char*>(rawPtr) + headerSize;
	Tge::Memory::TrackAllocation(header->category, size);
	return userPtr;
#else
	return rpaligned_alloc(static_cast<size_t>(align), size);
#endif // TGE_MEMORY_TRACKING_ENABLED
}

//////////////////////////////////////////////////////////////////////////
void* operator new[](size_t size, std::align_val_t align)
{
	return ::operator new(size, align);
}

//////////////////////////////////////////////////////////////////////////
void operator delete(void* ptr, std::align_val_t align) noexcept
{
	if (ptr != nullptr)
	{
#ifdef TGE_MEMORY_TRACKING_ENABLED
		size_t const alignment = static_cast<size_t>(align);
		size_t const headerSize = GetAlignedHeaderSize(alignment);

		void* rawPtr = static_cast<char*>(ptr) - headerSize;
		auto* header = static_cast<SAllocationHeader*>(rawPtr);

		Tge::Memory::TrackDeallocation(header->category, header->size);

		rpfree(rawPtr);
#else
		rpfree(ptr);
#endif // TGE_MEMORY_TRACKING_ENABLED
	}
}

//////////////////////////////////////////////////////////////////////////
void operator delete[](void* ptr, std::align_val_t align) noexcept
{
	::operator delete(ptr, align);
}

//////////////////////////////////////////////////////////////////////////
void operator delete(void* ptr, size_t, std::align_val_t align) noexcept
{
	::operator delete(ptr, align);
}

//////////////////////////////////////////////////////////////////////////
void operator delete[](void* ptr, size_t, std::align_val_t align) noexcept
{
	::operator delete(ptr, align);
}

//////////////////////////////////////////////////////////////////////////
void* operator new(size_t size, std::nothrow_t const&) noexcept
{
#ifdef TGE_MEMORY_TRACKING_ENABLED
	size_t const totalSize = sizeof(SAllocationHeader) + size;
	void* rawPtr = rpmalloc(totalSize);
	void* userPtr = nullptr;

	if (rawPtr != nullptr)
	{
		auto* header = static_cast<SAllocationHeader*>(rawPtr);
		header->category = Tge::Memory::GetCurrentCategory();
		header->size = size;

		userPtr = GetUserPtr(rawPtr);
		Tge::Memory::TrackAllocation(header->category, size);
	}

	return userPtr;
#else
	return rpmalloc(size);
#endif // TGE_MEMORY_TRACKING_ENABLED
}

//////////////////////////////////////////////////////////////////////////
void* operator new[](size_t size, std::nothrow_t const& tag) noexcept
{
	return ::operator new(size, tag);
}

//////////////////////////////////////////////////////////////////////////
void* operator new(size_t size, std::align_val_t align, std::nothrow_t const&) noexcept
{
#ifdef TGE_MEMORY_TRACKING_ENABLED
	size_t const alignment = static_cast<size_t>(align);
	size_t const headerSize = GetAlignedHeaderSize(alignment);
	size_t const totalSize = headerSize + size;

	void* rawPtr = rpaligned_alloc(alignment, totalSize);
	void* userPtr = nullptr;

	if (rawPtr != nullptr)
	{
		auto* header = static_cast<SAllocationHeader*>(rawPtr);
		header->category = Tge::Memory::GetCurrentCategory();
		header->size = size;

		userPtr = static_cast<char*>(rawPtr) + headerSize;
		Tge::Memory::TrackAllocation(header->category, size);
	}

	return userPtr;
#else
	return rpaligned_alloc(static_cast<size_t>(align), size);
#endif // TGE_MEMORY_TRACKING_ENABLED
}

//////////////////////////////////////////////////////////////////////////
void* operator new[](size_t size, std::align_val_t align, std::nothrow_t const& tag) noexcept
{
	return ::operator new(size, align, tag);
}
