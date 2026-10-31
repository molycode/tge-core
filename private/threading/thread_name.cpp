#include <tge/threading/thread_name.hpp>
#include <tge/platform.hpp>
#include <atomic>

#if defined(TGE_PLATFORM_LINUX)
#include <cstring>
#include <pthread.h>
#elif defined(TGE_PLATFORM_WINDOWS)
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
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
void SetCurrentThreadName(std::string_view name)
{
#if defined(TGE_PLATFORM_LINUX)
	// Linux caps the OS thread name at 15 characters plus a terminator; truncate to fit. The profiler hook
	// below still receives the full name, so only the htop/debugger label is shortened. Platforms without a
	// branch here get profiler-only naming until their OS path is added.
	constexpr size_t MaxOsNameLength = 15;
	char osName[MaxOsNameLength + 1];
	size_t const copyLength = (name.size() < MaxOsNameLength) ? name.size() : MaxOsNameLength;

	std::memcpy(osName, name.data(), copyLength);
	osName[copyLength] = '\0';
	pthread_setname_np(pthread_self(), osName);
#elif defined(TGE_PLATFORM_WINDOWS)
	std::wstring wideName{};
	int const numWide{ MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0) };

	wideName.resize(static_cast<size_t>(numWide));
	MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), wideName.data(), numWide);
	SetThreadDescription(GetCurrentThread(), wideName.c_str());
#endif // defined(TGE_PLATFORM_LINUX)

	ThreadNameHook const hook = gThreadNameHook.load(std::memory_order_relaxed);

	if (hook != nullptr)
	{
		hook(name);
	}
}
} // namespace Tge::Threading
