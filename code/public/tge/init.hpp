#pragma once

#include <cstddef>

namespace Tge
{
// Initialize all tge-core subsystems (Memory -> Threading -> IO)
// numThreads: 0 = auto-detect based on hardware_concurrency
bool Initialize(size_t numThreads = 0);

// Terminate all tge-core subsystems in reverse order (IO -> Threading -> Memory)
void Terminate();

// Per-frame update (resets frame allocator)
void Update();
} // namespace Tge
