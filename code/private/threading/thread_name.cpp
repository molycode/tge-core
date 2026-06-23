#include <tge/threading/thread_name.hpp>
#include <tge/platform.hpp>
#include <atomic>

#if defined(TGE_PLATFORM_LINUX)
#include <cstring>
#include <pthread.h>
#endif // defined(TGE_PLATFORM_LINUX)

namespace Tge::Threading
{
namespace
{
	std::atomic<ThreadNameHook> gThreadNameHook{ nullptr };
} // namespace

//////////////////////////////////////////////////////////////////////////
void SetThreadNameHook(ThreadNameHook hook)
{
	gThreadNameHook.store(hook, std::memory_order_relaxed);
}

//////////////////////////////////////////////////////////////////////////
void SetCurrentThreadName(char const* name)
{
#if defined(TGE_PLATFORM_LINUX)
	// Linux caps the OS thread name at 15 characters plus a terminator; truncate to fit. The profiler hook
	// below still receives the full name, so only the htop/debugger label is shortened. Other platforms
	// (Windows SetThreadDescription, etc.) get profiler-only naming until their OS path is added here.
	constexpr size_t MaxOsNameLength = 15;
	char osName[MaxOsNameLength + 1];
	size_t const nameLength = std::strlen(name);
	size_t const copyLength = (nameLength < MaxOsNameLength) ? nameLength : MaxOsNameLength;

	std::memcpy(osName, name, copyLength);
	osName[copyLength] = '\0';
	pthread_setname_np(pthread_self(), osName);
#endif // defined(TGE_PLATFORM_LINUX)

	ThreadNameHook const hook = gThreadNameHook.load(std::memory_order_relaxed);

	if (hook != nullptr)
	{
		hook(name);
	}
}
} // namespace Tge::Threading
