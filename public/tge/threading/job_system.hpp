#pragma once

#include <cstddef>
#include <limits>

namespace Tge::Threading
{
// Sizes the pool from hardware_concurrency. Any other count is taken literally, 0 included.
constexpr size_t AutoThreadCount{ std::numeric_limits<size_t>::max() };

size_t GetNumThreads();

void InitializeThread();
void FinalizeThread();
} // namespace Tge::Threading
