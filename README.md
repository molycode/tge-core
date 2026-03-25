# tge-core

A standalone C++23 foundation library providing logging, assertions, memory management, threading, math, and I/O utilities. Designed for use as a git submodule across multiple projects.

## Requirements

- C++23 compiler (GCC 15+ or Clang 22+)
- CMake 4.1+
- Ninja (recommended)

## Dependencies

Both are included as git submodules and require no separate installation:

- **[GLM](https://github.com/g-truc/glm)** — math types and operations
- **[rpmalloc](https://github.com/mjansson/rpmalloc)** — high-performance memory allocator (replaces global `new`/`delete`)

## Adding to Your Project

```bash
git submodule add <url> external/tge-core
git submodule update --init --recursive
```

In your `CMakeLists.txt`:

```cmake
add_subdirectory(external/tge-core)
target_link_libraries(MyApp PRIVATE TgeCore)
```

Use individual targets instead of `TgeCore` if you only need specific modules:

| Target | Description |
|--------|-------------|
| `TgeBase` | Header-only base utilities (assertions, color, non-copyable traits) |
| `TgeLogging` | Logging system |
| `TgeMemory` | Memory allocators and tracking |
| `TgeMath` | Math library (GLM-based) |
| `TgeThreading` | Job system and thread pool |
| `TgeIO` | File and path operations |
| `TgeInit` | Initialization/termination orchestration |
| `TgeCore` | Convenience target linking all of the above |

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `TGE_ENABLE_LOGGING` | `ON` | Enable the logging system (`TGE_LOGGING_ENABLED`) |
| `TGE_ENABLE_MEMORY_TRACKING` | `ON` | Enable per-category allocation tracking (`TGE_MEMORY_TRACKING_ENABLED`) |

Debug builds automatically define `TGE_DEBUG_ENABLED`, which enables assertions and debug checks.

## Initialization

All public headers use the `<tge/...>` include prefix.

```cpp
#include <tge/init/init.hpp>

int main()
{
    Tge::Initialize();      // Initializes memory → threading → IO

    // Per-frame update (resets the frame allocator, dispatches log listener callbacks)
    while (running)
    {
        Tge::Update();
        // ...
    }

    Tge::Terminate();       // Shuts down in reverse order
    return 0;
}
```

`Initialize()` accepts an optional thread count (`0` = auto-detect hardware concurrency).

---

## Modules

### Base Utilities

#### Assertions — `<tge/init/assert.hpp>`

```cpp
TGE_ASSERT(condition, "message")   // Checked in debug builds only
TGE_FATAL("message")               // Always triggers — logs and aborts
TGE_UNREACHABLE("message")         // Marks unreachable code paths
```

#### Non-copyable Traits — `<tge/non_copyable.hpp>`

Inherit to suppress copy and/or move operations with zero overhead (Empty Base Optimization):

```cpp
struct SNoCopy;        // Deletes copy ctor/assign, keeps move
struct SNoMove;        // Deletes move ctor/assign, keeps copy
struct SNoCopyNoMove;  // Deletes both copy and move
```

#### Color — `<tge/color.hpp>`

```cpp
Tge::SColor    color(255, 128, 0);         // 8-bit RGBA
Tge::SColorHDR hdr(1.0f, 0.5f, 0.0f);    // Float RGBA
```

#### Object Pool — `<tge/object_pool.hpp>`

Fixed-capacity, O(1) allocate and free, thread-safe:

```cpp
Tge::CObjectPool<MyObject, 128> pool;

MyObject* obj = pool.Allocate(arg1, arg2);  // Placement-new into pre-allocated slot
pool.Free(obj);                              // Explicit destructor + slot returned to pool

size_t used = pool.GetNumUsed();
size_t free = pool.GetNumFree();
```

The pool is non-copyable and non-movable. If no slots are available, `Allocate()` asserts.

---

### Logging — `Tge::Logging::`

#### Creating a logger — `<tge/logging/log.hpp>`

```cpp
#include <tge/logging/log.hpp>

// Typically declared as a file-scope or namespace-scope global
Tge::Logging::CLog gLog("MySystem", Tge::SColor(100, 200, 100));

gLog.Info("Server started on port {}", port);
gLog.Warning("Low memory: {} MiB remaining", remaining);
gLog.Error("Failed to open file: {}", path);
```

All methods accept `std::format`-style arguments. An optional `ETarget` parameter controls where output goes:

```cpp
gLog.Info(Tge::Logging::ETarget::Terminal, "Terminal only");
gLog.Error(Tge::Logging::ETarget::File,    "File only");
gLog.Warning(Tge::Logging::ETarget::All,   "Everywhere (default)");
```

`ETarget` is a bitflag: `Terminal`, `File`, `Console` (application listener), `All`, `None`.

The log system writes to `logs/tge_YYYY-MM-DD_HH-MM-SS.log` when initialized.

#### Log levels — `<tge/logging/log_level.hpp>`

`ELogLevel` is a bitflag: `Error`, `Warning`, `Info`, `All`, `None`. Per-channel levels can be controlled at runtime:

```cpp
Tge::Logging::GetLogSystem().SetLogLevel("MySystem", Tge::Logging::ELogLevel::Error);
Tge::Logging::GetLogSystem().SetAllLogLevels(Tge::Logging::ELogLevel::All);
```

A config file at `configs/logging.cfg` is loaded on `Initialize()`:

```ini
# configs/logging.cfg
default   = a          # All channels: all levels (e=error, w=warning, i=info, a=all, n=none)
MySystem  = ew         # MySystem: errors and warnings only
```

#### Listening to log output — `<tge/logging/log.hpp>`

Register a callback on a specific `CLog` instance to receive only that channel's messages:

```cpp
gMyLog.RegisterListener(this, [](Tge::Logging::SLogMessage const& msg)
{
    // msg.channelName  — std::string_view (use .data() not .c_str())
    // msg.message      — std::string
    // msg.level, msg.elapsedMs, msg.formattedTimestamp, etc.
});

// Replay buffered messages to a newly registered listener
gMyLog.FlushTo(this);

// On teardown
gMyLog.UnregisterListener(this);
```

Callbacks are dispatched from `Tge::Update()` — they are **not** called synchronously inside `Write()`. Only messages targeting `ETarget::Console` (or `ETarget::All`) reach listeners; `ETarget::Terminal` and `ETarget::File` messages are written immediately and never buffered.

If you are not using `Tge::Update()`, call `DispatchListeners()` manually each frame:

```cpp
Tge::Logging::GetLogSystem().DispatchListeners();
```

#### Pre-declared loggers — `<tge/logging/loggers.hpp>`

A global `gLog` instance is available for quick use without declaring your own:

```cpp
#include <tge/logging/loggers.hpp>

Tge::gLog.Info("Application version {}", version);
```

---

### Memory — `Tge::Memory::`

tge-core replaces the global `operator new`/`delete` with rpmalloc. When `TGE_MEMORY_TRACKING_ENABLED` is defined, every allocation is tagged with a category for tracking.

#### Allocator interface — `<tge/memory/allocator.hpp>`

```cpp
class IAllocator
{
    virtual void* Allocate(size_t size, size_t alignment) = 0;
    virtual void  Deallocate(void* ptr) = 0;

    virtual std::string_view GetName() const = 0;
    virtual size_t GetTotalAllocated() const = 0;
    virtual size_t GetCurrentUsage() const = 0;
    virtual size_t GetNumAllocations() const = 0;
};
```

`IAllocator` has a protected destructor — it is not intended to be deleted through the interface.

#### Provided allocators

| Header | Class | Description |
|--------|-------|-------------|
| `<tge/memory/default_allocator.hpp>` | `CDefaultAllocator` | General-purpose (rpmalloc). Global instance: `g_defaultAllocator` |
| `<tge/memory/linear_allocator.hpp>` | `CLinearAllocator` | Bump allocator, no individual frees. Global instance: `g_frameAllocator` (reset each `Tge::Update()`) |
| `<tge/memory/stack_allocator.hpp>` | `CStackAllocator` | LIFO — deallocations must reverse allocation order |
| `<tge/memory/pool_allocator.hpp>` | `CPoolAllocator` | Fixed-size blocks, fast alloc/free |
| `<tge/memory/free_list_allocator.hpp>` | `CFreeListAllocator` | Variable-size, first-fit strategy |

All concrete allocators inherit from `CTrackedAllocator` (`<tge/memory/tracked_allocator.hpp>`), which provides the atomic stat counters.

#### Using allocators with STL containers — `<tge/memory/std_allocator.hpp>`

```cpp
#include <tge/memory/std_allocator.hpp>
#include <tge/memory/linear_allocator.hpp>
#include <vector>

using FrameAlloc = Tge::Memory::CStdAllocator<int>;
std::vector<int, FrameAlloc> perFrameData(FrameAlloc(&Tge::Memory::g_frameAllocator));
```

#### Memory tracking — `<tge/memory/tracking.hpp>`

Two built-in categories exist (`Category::Global` for global new/delete, `Category::Other` for allocators). Register project-specific categories at startup:

```cpp
#include <tge/memory/tracking.hpp>

// Register once at application startup (name must have static lifetime)
Tge::Memory::CategoryId const catEnemies = Tge::Memory::RegisterCategory("Enemies");
Tge::Memory::CategoryId const catUI      = Tge::Memory::RegisterCategory("UI");

// Query stats at any time
Tge::Memory::SStats stats = Tge::Memory::GetStats(catEnemies);
size_t current = stats.GetCurrentUsage();

// Query category metadata
size_t           numCats = Tge::Memory::GetNumCategories();
std::string_view name    = Tge::Memory::GetCategoryName(catEnemies);

// Global totals across all categories
size_t totalAllocated   = Tge::Memory::GetTotalAllocated();
size_t totalDeallocated = Tge::Memory::GetTotalDeallocated();
size_t currentUsage     = Tge::Memory::GetCurrentUsage();
size_t numAllocations   = Tge::Memory::GetNumAllocations();

// Print all categories to the log
Tge::Memory::PrintStats();

// Reset all counters (e.g. between test runs)
Tge::Memory::Reset();
```

#### Scoped category context — `<tge/memory/category_context.hpp>`

Tag a block of allocations with a category (thread-local, zero-overhead when tracking is disabled):

```cpp
#include <tge/memory/category_context.hpp>

{
    Tge::Memory::CScopedCategory scope(catEnemies);
    // All global new/delete calls in this scope are attributed to catEnemies
    auto* enemy = new CEnemyAI();
}
```

---

### Math — `Tge::Math::`

#### Type aliases — `<tge/math/types.hpp>`

```cpp
Tge::Math::Vec2, Vec3, Vec4     // glm::vec2/3/4
Tge::Math::Mat2, Mat3, Mat4     // glm::mat2/3/4
Tge::Math::Quat                 // glm::quat
Tge::Math::IVec2, IVec3, IVec4  // Integer vectors
Tge::Math::UVec2, UVec3, UVec4  // Unsigned integer vectors
Tge::Math::DVec2, DVec3, DVec4  // Double precision vectors
```

Include `<tge/math/glm_include.hpp>` if you need direct GLM access. It configures GLM with radians, depth [0,1], and experimental extensions enabled.

#### Constants — `<tge/math/constants.hpp>`

```cpp
Tge::Math::Pi, TwoPi, HalfPi
Tge::Math::Epsilon, Infinity
Tge::Math::DegToRad, RadToDeg

Tge::Math::Vec3Zero, Vec3One
Tge::Math::Vec3Up, Vec3Down, Vec3Forward, Vec3Back, Vec3Right, Vec3Left
Tge::Math::Vec3UnitX, Vec3UnitY, Vec3UnitZ

Tge::Math::IdentityMat4, IdentityMat3, IdentityQuat
```

#### Functions — `<tge/math/functions.hpp>`

```cpp
Tge::Math::Clamp(value, min, max)
Tge::Math::Lerp(a, b, t)
Tge::Math::Normalize(v)
Tge::Math::Dot(a, b)
Tge::Math::Cross(a, b)
Tge::Math::Distance(a, b)
Tge::Math::Length(v)
Tge::Math::Reflect(incident, normal)
Tge::Math::Inverse(matrix)
Tge::Math::Transpose(matrix)
Tge::Math::ToRadians(degrees)
Tge::Math::ToDegrees(radians)

// Extract components from a matrix
Tge::Math::GetRightVector(m)
Tge::Math::GetUpVector(m)
Tge::Math::GetForwardVector(m)
Tge::Math::GetTranslation(m)
```

#### Transform — `<tge/math/transform.hpp>`

```cpp
Tge::Math::CTransform t;
t.position = {1.0f, 0.0f, 0.0f};
t.rotation = Tge::Math::QuaternionFromAxisAngle(Tge::Math::Vec3UnitY, Tge::Math::HalfPi);
t.scale    = {2.0f, 2.0f, 2.0f};

Tge::Math::Mat4 matrix = t.ToMatrix();
Tge::Math::CTransform recovered = Tge::Math::CTransform::FromMatrix(matrix);

// Matrix construction helpers
Tge::Math::Mat4 m = Tge::Math::Translate({1, 2, 3})
                  * Tge::Math::Rotate(angle, axis)
                  * Tge::Math::Scale({1, 1, 1});

Tge::Math::Mat4 view = Tge::Math::LookAt(eye, center, up);
Tge::Math::Mat4 proj = Tge::Math::Perspective(fovY, aspect, zNear, zFar);
```

#### Intersection — `<tge/math/intersection.hpp>`

```cpp
Tge::Math::SRay   ray{{0, 0, 0}, {0, 0, 1}};
Tge::Math::CAABB  box{{-1, -1, -1}, {1, 1, 1}};
Tge::Math::SSphere sphere{{0, 0, 0}, 1.0f};
Tge::Math::SPlane  plane{{0, 1, 0}, 0.0f};   // normal, distance

float tMin, tMax, t;

Tge::Math::Intersects(ray, box, tMin, tMax);    // Ray-AABB
Tge::Math::Intersects(ray, sphere, t);           // Ray-Sphere
Tge::Math::Intersects(ray, plane, t);            // Ray-Plane
Tge::Math::Intersects(box, sphere);              // AABB-Sphere
Tge::Math::Intersects(box, box);                 // AABB-AABB
Tge::Math::Intersects(sphere, sphere);           // Sphere-Sphere

Tge::Math::Contains(box, point);                 // Point in AABB
Tge::Math::Contains(sphere, point);              // Point in sphere

// CAABB convenience methods
Tge::Math::Vec3 center  = box.GetCenter();
Tge::Math::Vec3 extents = box.GetExtents();
Tge::Math::Vec3 size    = box.GetSize();
```

---

### Threading — `Tge::Threading::`

#### Job groups — `<tge/threading/job_group.hpp>`

```cpp
#include <tge/threading/job_group.hpp>

Tge::Threading::CJobGroup group;

// Submit lambdas
group.SubmitJob([]() { /* work */ });
group.SubmitJob([]() { /* work */ }, Tge::Threading::EJobPriority::High);

// Submit IJob subclasses
group.SubmitJob(std::make_unique<MyJob>());

group.Wait();                   // Block until all jobs in this group complete
bool done = group.IsComplete();
size_t n  = group.GetActiveCount();
```

A pre-initialized `g_defaultJobGroup` is available for fire-and-forget submissions.

`CJobGroup` is non-copyable and non-movable. Use `std::unique_ptr<CJobGroup>` if dynamic lifetime is needed.

#### Implementing a job — `<tge/threading/job.hpp>`

```cpp
class CMyJob final : public Tge::Threading::IJob
{
public:
    void Execute() override
    {
        // Runs on a worker thread
    }
};
```

#### Thread pool queries — `<tge/threading/job_system.hpp>`

```cpp
size_t threads = Tge::Threading::GetNumThreads();
```

#### MPSC queue — `<tge/threading/mpsc_queue.hpp>`

Lock-free multi-producer single-consumer queue. Multiple threads may call `Enqueue()` concurrently; only one thread may call `Dequeue()`:

```cpp
Tge::Threading::CMpscQueue<MyEvent> queue;

// Producer threads
queue.Enqueue(event);

// Consumer thread only
MyEvent e;
while (queue.Dequeue(e)) { /* process e */ }
```

---

### IO — `Tge::IO::`

#### File operations — `<tge/io/file.hpp>`

```cpp
std::string text;
Tge::IO::CFile::ReadAllText("data/config.txt", text);

std::vector<uint8_t> bytes;
Tge::IO::CFile::ReadAllBytes("data/model.bin", bytes);

Tge::IO::CFile::WriteAllText("output.txt", "hello");

size_t   size  = Tge::IO::CFile::GetFileSize("data/model.bin");
uint64_t mtime = Tge::IO::CFile::GetLastWriteTime("data/config.txt");
bool     exists = Tge::IO::CFile::Exists("data/config.txt");
```

#### Path utilities — `<tge/io/path.hpp>`

```cpp
Tge::IO::CPath::Join("data", "config.txt")             // "data/config.txt"
Tge::IO::CPath::GetDirectory("data/config.txt")        // "data"
Tge::IO::CPath::GetFilename("data/config.txt")         // "config.txt"
Tge::IO::CPath::GetExtension("data/config.txt")        // ".txt"
Tge::IO::CPath::GetFilenameWithoutExtension("a.txt")   // "a"
Tge::IO::CPath::Normalize("data/../data/./config.txt") // "data/config.txt"
Tge::IO::CPath::GetAbsolutePath("data/config.txt")
Tge::IO::CPath::IsAbsolute("/absolute/path")           // true
```

#### Directory operations — `<tge/io/directory.hpp>`

```cpp
Tge::IO::CDirectory::Create("output/logs");

std::vector<std::string> files;
Tge::IO::CDirectory::GetFiles("data", files, /* recursive */ true);

std::string cwd = Tge::IO::CDirectory::GetCurrentWorkingDirectory();
std::string exe = Tge::IO::CDirectory::GetExecutableDirectory();
```

#### Binary streams — `<tge/io/stream.hpp>`

```cpp
// Write
Tge::IO::CBinaryWriter writer;
writer.Write<uint32_t>(42);
writer.Write<float>(3.14f);
Tge::IO::CFile::WriteAllBytes("out.bin", writer.GetData().data(), writer.GetData().size());

// Read
std::vector<uint8_t> data;
Tge::IO::CFile::ReadAllBytes("out.bin", data);
Tge::IO::CBinaryReader reader(data);
uint32_t val = reader.Read<uint32_t>();
```

---

## Namespace Summary

| Namespace | Contents |
|-----------|----------|
| `Tge::` | Init, assertions, color, non-copyable traits, object pool |
| `Tge::Logging::` | `CLog`, `CLogSystem`, `SLogMessage`, `ELogLevel`, `ETarget` |
| `Tge::Memory::` | Allocators, tracking, `CategoryId`, `CScopedCategory` |
| `Tge::Math::` | Types, constants, functions, `CTransform`, `CAABB`, intersection tests |
| `Tge::Threading::` | `CJobGroup`, `IJob`, `CMpscQueue`, `EJobPriority` |
| `Tge::IO::` | `CFile`, `CPath`, `CDirectory`, `CBinaryReader`, `CBinaryWriter` |
