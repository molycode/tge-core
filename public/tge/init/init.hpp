#pragma once

#include <tge/threading/job_system.hpp>

#include <cstddef>

namespace Tge
{
// Initialize the tge-core subsystems that need it, in dependency order.
// numThreads: AutoThreadCount sizes the pool from the hardware; 0 runs every job on the main thread.
bool Initialize(size_t numThreads = Threading::AutoThreadCount);

// Terminate them in reverse order.
void Terminate();

// Once per frame: runs a share of the queued jobs when there are no workers, then delivers queued log lines.
void Update();
} // namespace Tge
