#pragma once

#include <cstddef>

namespace Tge::Memory
{
// Vendor-neutral allocation observer. The global operator new/delete report every allocation through
// NotifyAllocation/NotifyDeallocation; a profiler (Tracy, etc.) registers via SetAllocationHooks so the
// allocator never depends on a specific backend. A registered hook MUST NOT allocate through global new
// (it would recurse) — profiler clients use their own allocator, which satisfies this.
using AllocationHook = void(*)(void* ptr, size_t size);
using DeallocationHook = void(*)(void* ptr);

// Register or clear the observer (pass nullptr to clear). Storage is atomic, so this is race-free; in
// practice it is set once before worker threads spawn and cleared at shutdown before the profiler client
// is destroyed.
void SetAllocationHooks(AllocationHook onAlloc, DeallocationHook onFree);

// Called by the global operators. A no-op (single relaxed atomic load + branch) when no hook is registered
// or the pointer is null.
void NotifyAllocation(void* ptr, size_t size);
void NotifyDeallocation(void* ptr);
} // namespace Tge::Memory
