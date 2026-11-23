#include <tge/threading/thread_name.hpp>
#include "threading/os_thread_name.hpp"

#include <atomic>

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
	SetOsThreadName(name);

	ThreadNameHook const hook = gThreadNameHook.load(std::memory_order_relaxed);

	if (hook != nullptr)
	{
		hook(name);
	}
}
} // namespace Tge::Threading
