#pragma once

#include <cstddef>

namespace Tge::Threading
{
size_t GetNumThreads();

void InitializeThread();
void FinalizeThread();
} // namespace Tge::Threading
