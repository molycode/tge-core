#pragma once

#include <tge/threading/job_system.hpp>

#include <cstddef>

namespace Tge
{
// Initialize the tge-core subsystems that need it, in dependency order.
// numThreads: AutoThreadCount sizes the pool from the hardware, 0 asks for no worker threads at all.
bool Initialize(size_t numThreads = Threading::AutoThreadCount);

// Terminate them in reverse order.
void Terminate();

// Per-frame update (resets frame allocator)
void Update();
} // namespace Tge
