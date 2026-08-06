#pragma once

#include <string_view>

namespace Tge::Threading
{
// Vendor-neutral thread naming. SetCurrentThreadName sets the OS thread name (visible in debuggers/htop)
// and forwards to a registered profiler hook (Tracy, etc.), so threading code never depends on a specific
// profiler. Call it once from each worker's entry point.
using ThreadNameHook = void(*)(std::string_view name);

// Register or clear the profiler hook (pass nullptr to clear). Set before worker threads spawn so their
// entry-point naming reaches the profiler.
void SetThreadNameHook(ThreadNameHook hook);

// Names the calling thread: sets the OS thread name (capped per platform — 15 chars on Linux, truncated to
// fit) and forwards the full name to the registered profiler hook. The name is consumed synchronously, so
// passing a temporary (e.g. std::format(...)) is safe.
void SetCurrentThreadName(std::string_view name);
} // namespace Tge::Threading
