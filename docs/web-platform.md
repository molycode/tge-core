# The web as a platform

How the module pool runs in a browser (Emscripten, WebAssembly, WebGPU), and where each part of that belongs. It
extends [modular-architecture.md](modular-architecture.md): nothing here changes the layering, it says where a
third platform goes inside it.

Status: **design agreed and the platform layer shipped 2026-11-23** (Core and Window build for the web); a spike
answered the backend's questions, listed at the end. Core's frame-loop host and user folders followed on 2026-11-25.

## Two axes, never one

**The web is a platform, and WebGPU is a graphics backend.** The engines surveyed in 2026-11 (Unity, Unreal's HTML5
platform and its WebGPU successors, Godot, Bevy) all keep them apart, and Tge does too:

- the **platform** is the entry point, the frame loop, files and persistence, threads, sockets turned into fetch and
  WebSocket, audio, input that the browser only grants inside a user gesture, and the loader page;
- the **backend** is how a renderer reaches the GPU. A WebGPU backend runs in a browser and natively (through Dawn on
  Vulkan, D3D12 or Metal), so it is not tied to the web, and the web is not tied to one renderer.

## Where web code lives

**In two places: platform services in Core, and each module's own web backend behind the interface it already has.**
There is no platform repository. A `platform/web/` folder needs an engine to sit in, and in this pool there is no
engine; a module that needs the browser carries that code itself, the way it carries its Linux and Windows code today.

| Concern | Lives in | Shape |
|---|---|---|
| Platform identity | Core's `platform.hpp`, a `cmake/toolchains/web/` file in each repository that builds for the web | `TGE_PLATFORM_WEB`; each platform's implementation in a file of its own, picked by the build |
| Files, user folders, persistence, logging, thread names | Core, per-platform branches in `private/` | POSIX branches reused where the browser's file system behaves alike; own branches for the executable folder, user folders on a persistent mount, and the flush that makes a write durable |
| Heap | Core Memory | the C runtime's allocator; rpmalloc never builds for the web |
| Jobs and threads | Core Threading | two build variants, below |
| Frame loop | Core Lifecycle | one host that runs a loop on the desktop and hands the frame to the browser on the web, below |
| Window, input, canvas | Window's GLFW backend, with a web implementation | the canvas as a native-window kind in Core's surface vocabulary; gesture-gated requests (pointer lock, fullscreen) fulfilled inside the next input event; on touch devices, touches as pointer events in Window's own input vocabulary once a title targets them, what a touch means left to the app |
| GPU | the Renderer, a WebGPU backend as a component beside the Vulkan one | one `IRenderer`; the backend-neutral logic shared; capabilities queried at run time |
| Shaders | the Renderer's shader compiler | one source, translated offline per backend |
| Audio | the audio module's own backend | the browser's audio API; threadless, its decoding jobs pumped by hand |
| HTTP | the net module, a second transport behind its client | fetch; the client's API is asynchronous already |
| Content delivery | the app's packaging first; an asynchronous content service in Core later | below |
| Browser glue (JavaScript) | each module's own library, attached by its own target | no module reaches into another's glue |
| Loader page, compression, headers | each app's release tooling | feature detection before start |
| Bridges | tge-integration | none new: the window's bridge hands a canvas to the renderer as it hands a window today |

## Rules

- **Platform specifics are separated physically, not by `#ifdef`.** Each platform's implementation is a file of its
  own beside the shared code (`poller_linux.cpp`, `poller_windows.cpp`, `poller_web.cpp`), and the build compiles the
  one for the target; headers and shared code carry no platform `#if`. A unit that gains the web is first split into
  per-platform files, then gets its web file. A build option that swaps an implementation is handled the same way.
- **Public headers never change shape per platform.** What the web adds to an interface is vocabulary every platform
  compiles (a canvas window kind, a canvas selector setting that other platforms ignore), never an `#ifdef`'d member.
- **A contract the web needs asynchronous is asynchronous everywhere.** Device creation, GPU readback, pipeline
  creation, persistence and, later, content reads complete through polling or callbacks on every platform; on the
  desktop they may complete at once. Blocking through the browser's promise integration is the exception for what
  cannot be restructured, not the design.
- **A lower tier drops features, never changes the look.** The WebGPU backend keeps the Vulkan backend's shading
  model, units, exposure and tone map; what it cannot do (ray tracing, bindless, multi-draw) it leaves out or replaces
  with a precomputed equivalent. A renderer tier that lights the same scene differently pushes per-renderer fixes into
  every game.
- **Platform code waits for its first consumer.** A module gains its web branch when something builds it for the web,
  not before.

## Threads: two build variants, threadless the default

A threaded build needs the page to be cross-origin isolated (COOP and COEP headers), which every host and every
embedding page must then provide and which cuts off third-party content that does not opt in. Godot required threads
in 4.0 and made a threadless build its default again in 4.3 for that reason. So:

- **threadless is the default**: the job system has no workers; a group's `Wait()` runs its own jobs, and groups that
  are only polled are drained on the main thread by Core's frame tick within a time budget. It runs on any host;
- **threaded is opt-in** for heavier titles: the pool is created up front (the browser cannot start a thread on demand
  without returning to its event loop), and the host must send the isolation headers.

Modules are written once for both: they submit work to the job system and poll or wait for it. A module that runs a
thread of its own on the desktop (an event loop, an audio device) uses the browser's own mechanism on the web instead.

## The frame loop

Every app owns a blocking loop today (`while (gRuntime->CanRun()) { gRuntime->Update(dt); }`). A browser never lets
the main thread block: the page registers a frame callback and returns. **The loop is inverted once, in Core's
Lifecycle, beside `IRuntime`**: a host that runs the loop on the desktop and registers the frame with the browser on
the web. The app keeps everything it does per frame; it stops owning the mechanism. App state must then not live on
`main`'s stack, which the browser unwinds.

The host is `RunFrameLoop(std::unique_ptr<IApp>)` (`tge/module/frame_loop.hpp`): the app is handed over, and
`Initialize`, `Update` while `CanRun`, and `Terminate` are called on it, whose status is the program's. On the web the
call returns once the app is initialized, so nothing that outlives it may hang on it, a destructor in `main` included.

Startup and shutdown follow from it. Acquiring a WebGPU device completes asynchronously, so a backend accepts work
before its device exists and defers it, the same deferral that lets workers hand it data (GPU objects live on one
thread in a browser); what starts late starts across the app's updates. Shutdown waits for the last persistence flush,
then ends the program, which a page sees (`onExit`) when it is linked with `-sEXIT_RUNTIME`.

## User folders and persistence

A browser has no disk: files live in memory, where reads and writes are synchronous as everywhere, and what is to
outlive the page goes to its IndexedDB. `GetUserFolder` answers with folders of a mount of Emscripten's **IDBFS**:

- it is **loaded before `main`**, so the app's first read finds what the last run wrote;
- it is **stored by Core's frame tick** (`Tge::Update`) after a change and by `RunFrameLoop` at the end, each time in one
  IndexedDB transaction: a page that dies leaves the state before a store or after it, and `WriteFileAtomically` keeps
  a store from meeting half a file;
- a page whose browser keeps nothing (no IndexedDB, refused, out of quota) runs on the folders in memory and says so
  once; a store that could not be loaded is never written to.

**IDBFS rather than OPFS**: Emscripten reaches OPFS through WasmFS alone, by a thread of its own or the browser's promise
integration, which the threadless default has neither of; and it has no transaction over several files. IDBFS costs the
folders' size in memory, small for settings and saves.

Only a program that calls `GetUserFolder` loads them. Two pages of one origin share the store, and the last to store wins,
whole; caches and logs that grow belong outside the user folders there. `WriteFileAtomically` does not `fsync` on the web:
Emscripten's would store by itself, and deadlock a main thread that waits for the worker calling it.

## Content delivery

The first web build preloads its content into the browser's in-memory file system, which keeps every synchronous read
in the pool working unchanged; it costs one up-front download and the memory it occupies. When content outgrows that,
reads move to an asynchronous content service in Core that fetches on demand and caches through the browser's cache,
read by the asset loader, the renderer and the UI alike. Texture formats follow the device: block-compressed on the
desktop, transcoded or per-device variants where the adapter lacks them.

## What the spike answered (2026-11-23)

- **The device arrives by callbacks**, processed once a frame, so neither the browser's promise integration nor
  Asyncify is needed; a backend queues what it is given until the device exists.
- **Both thread variants run**: threadless on any page, threaded on a cross-origin isolated one (elsewhere it stops at
  start, unable to share its memory).
- **The binding model without bindless fits**: four groups (frame, material, lights, shadows), one binding per material
  texture slot and one sampler, per-draw data in the frame group. A material shader then needs more sampled textures
  than the spec's default and requests the adapter's limit; four groups is the most Dawn offers.
- **The shaders translate offline**: GLSL to SPIR-V to WGSL through Tint, with bit-pattern NaN tests (WGSL has none),
  derivative uniformity switched off, the samplers Tint splits from combined ones placed from a binding table, and
  barrier loops bounded by values WGSL can prove uniform. Ray-traced and layered-rendering shaders have no web form.
- **Dawn natively** uses the prebuilt package of the revision the browser port pins, so one `webgpu.h` serves both.
- **Pointer lock and pixel density hold with a person at the browser**: a game played in Firefox on a scaled desktop
  (2026-11-25) captured and released the mouse and put the cursor where the pointer was.
- **Open still**: fullscreen through the GLFW emulation wants a person at a browser.
