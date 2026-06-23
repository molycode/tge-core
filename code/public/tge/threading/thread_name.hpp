#pragma once

namespace Tge::Threading
{
// Vendor-neutral thread naming. SetCurrentThreadName sets the OS thread name (visible in debuggers/htop)
// and forwards to a registered profiler hook (Tracy, etc.), so threading code never depends on a specific
// profiler. Call it once from each worker's entry point.
using ThreadNameHook = void(*)(char const* name);

// Register or clear the profiler hook (pass nullptr to clear). Set before worker threads spawn so their
// entry-point naming reaches the profiler.
void SetThreadNameHook(ThreadNameHook hook);

// Names the calling thread. 'name' must be null-terminated — both the OS API and profiler clients require a
// C string, so string_view is unsuitable here. The OS name is capped per platform (15 chars on Linux) and
// truncated to fit; the full name is always passed to the profiler hook.
void SetCurrentThreadName(char const* name);
} // namespace Tge::Threading
