# tge-core - Standalone Foundation Library

## Overview
**tge-core** is a standalone C++23 foundation library extracted from Tge (The Game Engine). It provides logging, assertions, memory management (with rpmalloc global new/delete override), threading (job system), math (GLM-based), and I/O utilities. Designed to be shared across multiple projects (Tge, msgr, etc.) as a git submodule.

## Architecture

### Namespace Structure
All types live under `Tge::` with feature-based sub-namespaces:
- `Tge::` - Base utilities (SNoCopyNoMove, SColor, FatalError, gLog)
- `Tge::Logging::` - Log system (CLog, CLogSystem, ELogLevel)
- `Tge::Memory::` - Allocators, tracking (CDefaultAllocator, CLinearAllocator, etc.)
- `Tge::Math::` - GLM wrappers, transforms, intersections (Vec3, Mat4, STransform, SAABB, SFrustum)
- `Tge::Threading::` - Job system (CJobGroup, CThreadPool, IJob)
- `Tge::IO::` - File, path, directory, binary streams (CFile, CPath, CDirectory)
- `Tge::Geometry::` - Mesh vocabulary (SVertex, SSkinVertex, SMorphDelta, EPrimitiveTopology)
- `Tge::Material::` - Material vocabulary (SMaterialProperties, STextureTransform, ETextureSlot)
- `Tge::Profiling::` - Profiling markers and the backend hook wiring (CScopedPlotMs, RegisterHooks)

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
| TgeProfiling | INTERFACE | TgeBase, TgeMemory, TgeThreading, Tracy (optional) | `TGE_PROFILE_*` markers; no-ops without a Tracy client target |
| TgeModule | STATIC | TgeBase | Module contract: IModule, IModuleId, SDependency, EFramePhase, SRunContext |
| TgeLifecycle | STATIC | TgeModule, TgeLogging, TgeInit, TgeProfiling | Dependency ordering + phase dispatch (IRuntime); linked by a composition root |
| TgeGeometry | INTERFACE | TgeMath | Mesh vocabulary shared by producers (loaders) and consumers (renderers) |
| TgeMaterial | INTERFACE | TgeMath | Material vocabulary shared by producers and consumers |
| TgeCore | INTERFACE | all above | Convenience all-in-one |

`TgeGeometry` and `TgeMaterial` are engine-domain vocabulary rather than general infrastructure. They live
here because both a loader and a renderer embed these types **by value**, so they can never version
independently of each other — the property that defines core. Consumers who want only infrastructure should
link the specific libs (`TgeBase`, `TgeLogging`, …) rather than the `TgeCore` umbrella.

`TgeModule` and `TgeLifecycle` are split for the same reason they were two targets in the engine: a module
needs only the contract, while driving modules is the composition root's job. `TgeCore` therefore carries
`TgeModule` and not `TgeLifecycle`. The cost of hosting the contract here is that it versions with core — any
change to `IModule` or `EFramePhase` is a core release every module rebuilds against.

### Initialization
```cpp
#include <tge/init.hpp>

int main()
{
    Tge::Initialize();  // Memory -> Threading
    // ... application code ...
    Tge::Update();      // Per-frame (resets frame allocator)
    Tge::Terminate();   // Threading -> Memory
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
Requires CMake 4.x (`cmake --version` to verify — system cmake may be too old).

```bash
cmake --preset tge-core-linux-gcc_16-debug
cmake --build --preset tge-core-linux-gcc_16-debug
```

## Remote Setup

**GitHub is the primary dev repo. Gitea is a read-only mirror that follows it — never push to Gitea directly.**

The submodule is cloned from Gitea (`.gitmodules` URL), so `origin` lands on Gitea. Ignore `origin` for pushes — always use the `github` remote.

On each new clone or session, add the `github` remote and configure it to dual-push to both destinations:

```bash
git remote add github git@github-molycode:molycode/tge-core.git
git remote set-url --add --push github git@github-molycode:molycode/tge-core.git
git remote set-url --add --push github gitea@git.satoki.org:moly/tge-core.git
```

After this, `git push github main` sends to GitHub first and mirrors to Gitea automatically.

## Coding Standards
Follow global C++ guidelines from `~/.claude/CLAUDE.md` (Tge project conventions).
