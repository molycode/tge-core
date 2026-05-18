# tge-core - Standalone Foundation Library

## Overview
**tge-core** is a standalone C++23 foundation library extracted from Tge (The Game Engine). It provides logging, assertions, memory management (with rpmalloc global new/delete override), threading (job system), math (GLM-based), and I/O utilities. Designed to be shared across multiple projects (Tge, msgr, etc.) as a git submodule.

## Architecture

### Namespace Structure
All types live under `Tge::` with feature-based sub-namespaces:
- `Tge::` - Base utilities (SNoCopyNoMove, SColor, FatalError, gLog)
- `Tge::Logging::` - Log system (CLog, CLogSystem, ELogLevel)
- `Tge::Memory::` - Allocators, tracking (CDefaultAllocator, CLinearAllocator, etc.)
- `Tge::Math::` - GLM wrappers, transforms, intersections (Vec3, Mat4, STransform, SAABB)
- `Tge::Threading::` - Job system (CJobGroup, CThreadPool, IJob)
- `Tge::IO::` - File, path, directory, binary streams (CFile, CPath, CDirectory)

### Include Paths
All public headers use `<tge/...>` prefix:
```cpp
#include <tge/math/types.hpp>
#include <tge/logging/log.hpp>
#include <tge/memory/allocator.hpp>
#include <tge/init.hpp>
```

### CMake Targets
| Target | Type | Dependencies | Description |
|--------|------|-------------|-------------|
| TgeBase | INTERFACE | none | Header-only base utilities |
| TgeLogging | STATIC | TgeBase | Logging system |
| TgeMemory | STATIC | TgeBase, rpmalloc, TgeLogging | Memory allocators and tracking |
| TgeMath | STATIC | TgeBase, GLM | Math library |
| TgeThreading | STATIC | TgeBase, rpmalloc | Job system and thread pool |
| TgeIO | STATIC | TgeBase | File and path operations |
| TgeInit | STATIC | TgeBase, TgeMemory, TgeThreading, TgeIO, TgeLogging | Initialization/termination |
| TgeCore | INTERFACE | all above | Convenience all-in-one |

### Initialization
```cpp
#include <tge/init.hpp>

int main()
{
    Tge::Initialize();  // Memory -> Threading -> IO
    // ... application code ...
    Tge::Update();      // Per-frame (resets frame allocator)
    Tge::Terminate();   // IO -> Threading -> Memory
}
```

### Build Defines
- `TGE_DEBUG_ENABLED` - Debug assertions and logging (auto-enabled in Debug builds)
- `TGE_LOGGING_ENABLED` - Logging system (CMake option, default ON)
- `TGE_MEMORY_TRACKING_ENABLED` - Memory tracking (CMake option, default ON)

### Dependencies
- **GLM** (git submodule) - Math library
- **rpmalloc** (git submodule) - Fast memory allocator (global new/delete override)

## Build
```bash
cmake --preset tge-core-linux-gcc16-debug
cmake --build --preset tge-core-linux-gcc16-debug
```

## Remote Setup

Two remotes: `github` (primary, public) and `hosty` (Gitea mirror). New commits go on `github`; `hosty` stays in sync via dual-push.

On each new clone or session, configure pushes to go to both simultaneously:

```bash
git remote set-url --add --push github git@github-molycode:molycode/tge-core.git
git remote set-url --add --push github gitea@git.satoki.org:moly/tge-core.git
```

After this a single `git push` sends to both. Fetch still pulls from GitHub only.

## Coding Standards
Follow global C++ guidelines from `~/.claude/CLAUDE.md` (Tge project conventions).
