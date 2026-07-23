#pragma once

#include <tge/config.hpp>

#include <tge/profiling/profiling.hpp>

#if defined(TGE_PROFILE_TRACY)
#include <tge/memory/allocation_hooks.hpp>
#include <tge/threading/thread_name.hpp>
#include <tracy/Tracy.hpp>
#include <string>
#endif // defined(TGE_PROFILE_TRACY)

// Process-level wiring between TGE's vendor-neutral hooks (tge-core's allocation observer + thread naming)
// and the active profiler backend. This is the ONE place the Tracy memory/thread-name APIs are referenced —
// same "never marry a vendor" seam as profiling.hpp. RegisterHooks() must run before any worker thread
// spawns so its entry-point naming reaches the profiler; UnregisterHooks() must run at shutdown before the
// profiler client is torn down (late static-dtor allocations then hit a null hook, which is harmless).
namespace Tge::Profiling
{
#if defined(TGE_PROFILE_TRACY)
namespace Detail
{
	inline void TracyOnAllocation(void* ptr, size_t size)
	{
		TracyAlloc(ptr, size);
	}

	inline void TracyOnDeallocation(void* ptr)
	{
		TracyFree(ptr);
	}

	inline void TracyOnThreadName(std::string_view name)
	{
		tracy::SetThreadName(std::string{ name }.c_str());
	}
} // namespace Detail

inline void RegisterHooks()
{
	Tge::Memory::SetAllocationHooks(&Detail::TracyOnAllocation, &Detail::TracyOnDeallocation);
	Tge::Threading::SetThreadNameHook(&Detail::TracyOnThreadName);
}

inline void UnregisterHooks()
{
	Tge::Memory::SetAllocationHooks(nullptr, nullptr);
	Tge::Threading::SetThreadNameHook(nullptr);
}
#else
inline void RegisterHooks() {}
inline void UnregisterHooks() {}
#endif // defined(TGE_PROFILE_TRACY)
} // namespace Tge::Profiling
