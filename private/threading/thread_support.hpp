#pragma once

#include <cstddef>

namespace Tge::Threading
{
// How many worker threads this build can run at most, and how many AutoThreadCount stands for.
size_t GetMaxWorkerThreads();
size_t GetAutoWorkerThreads();
} // namespace Tge::Threading
