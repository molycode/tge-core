#pragma once

#include <cstddef>

namespace Tge
{
// Initialize the tge-core subsystems that need it, in dependency order.
// numThreads: 0 = auto-detect based on hardware_concurrency
bool Initialize(size_t numThreads = 0);

// Terminate them in reverse order.
void Terminate();

// Per-frame update (resets frame allocator)
void Update();
} // namespace Tge
