# Run contexts and optional dependencies

Step 1 of [modular-architecture.md](modular-architecture.md): make "nothing is mandatory except Core"
expressible and enforced at load time. A **run context** names the set of modules to load and the schedule
they tick in; a module declares which siblings it requires, which it merely uses, and which it only wants to
be told about.

Status: **shipped.** The mechanism, the `default` and `server` contexts, and — through step 2 — a Scene and
Animation that run with the renderer absent. The `server` context boots from its own executable, spawns a
model and a skinned mesh, ticks and exits cleanly; it is no longer merely a unit-test subject. That executable
now lives in its own repository (`moly/tge-server`), so the two contexts this document compares are authored
in two repositories rather than two directories. This document has been updated to describe what shipped, which in a few places differs from
the design it originally proposed — those places are called out inline.

## The problem it solved

Kept because the design only makes sense against it. Originally `--headless` set `gSettings.headlessMode`, and
the Window and Renderer modules initialized into a deliberately inert state, leaving `gWindow` and `gRenderer`
null while remaining registered and ticking. Every other module still assumed they were there, so the process
segfaulted during `Debug` initialization, before the first frame:

```
#0  CLightingDialog::SyncSunStateFromRenderer   lighting_dialog.cpp:451
#1  CLightingDialog::Initialize                 lighting_dialog.cpp:66
#2  CSystem::Initialize                         system.cpp:55
#3  Debug::CModule::Initialize                  module.cpp:26
#4  CRuntime::InitializeModules                     runtime.cpp:261
```

Guarding that one site moved the crash to the next of **268 unguarded `gRenderer->` dereferences**. Compiling
`TGE_DEBUG_UI_ENABLED` off moved the first crash to `CScene::Update`. The crash site tracked build flags,
which is the signature of a missing concept rather than a missing null check.

**The missing concept was optional dependency.** `GetDependencies()` returned a flat span and the runtime
treated every entry as required — an unregistered dependency was fatal. There was no way to say "I can run
without this", so the only available expression of absence was a null global that nothing was obliged to check.

## The finding that shaped the design

`GetDependencies()` conflated three different relations, and because of that **the dependency graph had
cycles**:

```
Window    → Debug, Events
Renderer  → Window, Debug
Debug     → AssetLoader, Renderer, Events      ⇒  Renderer → Debug → Renderer
                                               ⇒  Window → Debug → Renderer → Window
```

The cycles are invisible today only because the runtime never orders anything by the graph — init order is
registration order, and `GetDependencies()` is used solely to validate registration and to fire
`OnDependencyInitialized`. Window and Renderer were the only consumers of that callback, and both used it for
the same thing: registering console commands once Debug is up. That is a **notification**, not an ordering
constraint. (Both have since been severed the other way — Debug registers those commands itself, so no module
declares a `Notify` on Debug any more.)

Splitting the relations breaks the cycles and makes the graph orderable:

```cpp
enum class EDependencyKind : uint8_t
{
	Required,   // must be present and initialized before me; absent means I cannot load
	Optional,   // I use it when present and cope when absent; ordered before me when present
	Notify      // tell me when it initializes or terminates; NOT an ordering constraint
};

struct SDependency final
{
	IModuleId*      pModule{ nullptr };
	EDependencyKind kind{ EDependencyKind::Required };
	SVersion        minVersion{};   // added by versioning; unset is satisfied by any version
};
```

(`minVersion` is not part of step 1 — versioning added it later, and `CModuleGraph::Resolve` now also checks a
semver-compatible constraint and the module-contract version. See [modular-architecture.md](modular-architecture.md).)

Restated against the real modules, the `Required`/`Optional` subgraph is acyclic:

| Module | Required | Optional | Notify |
|---|---|---|---|
| Core, Events, AssetLoader | — | — | — |
| Window | Events | — | — |
| Renderer | Window, **AssetLoader** | — | — |
| Scene | Core, AssetLoader | **Renderer** | — |
| Animation | Core, Events, AssetLoader | **Renderer** | — |
| Debug | AssetLoader, Renderer, Events, **Window** | — | — |
| *Network (planned)* | *Core, Events* | — | — |

Network is listed because it is the first module that will be natively presentation-agnostic, which makes it
the mechanism's best validation: if the design is right, adding it should require touching no existing module.

`Renderer → AssetLoader` is new but not invented: the renderer calls `gAssetLoader` in six places during
teardown, and that constraint is currently enforced only by hand-ordering the registration list and a comment
warning not to disturb it. Declaring it lets the sort enforce what the comment asks for.

## Contexts

A context is a static table — no parser, no new failure mode at boot, and the compiler checks it.

```cpp
struct SScheduleEntry final
{
	EFramePhase phase{ EFramePhase::FrameStart };
	IModule*    pModule{ nullptr };
};

struct SRunContext final
{
	std::span<IModule* const>       modules;
	std::span<SScheduleEntry const> schedule;
	std::span<IModule* const>       noTick;
};
```

**Changed from the original proposal:** the struct shipped without a `name` field and without an
`EContextTrait traits` field (Context traits, below, went unbuilt), and it names modules by `IModule*` — the
drivable handle a composition root holds — rather than by `IModuleId*`. The `name` was the lookup key of a
contexts-as-data model that did not survive: once each root hands its context straight to `Initialize`, the
name has no job.

The type is engine vocabulary and lives in tge-core's `tge/module/run_context.hpp`, beside `IModule`. **The
tables are authored by each composition root** — the demo, the server and the rendering tests each define
their own `GetRunContext()`. They were engine-private at first, then moved to the roots, because a context is
the statement of what a particular product loads, which makes it **policy owned by the game**. The
distribution model says the same from the other side: a static library contributes only what something
references, so the table naming a module is what links it — engine-private tables naming all eight modules
would make every game link Vulkan and GLFW.

The two contexts that ship:

- **Default** (the demo) — every module; the schedule exactly as before the mechanism. Behaviour-identical,
  verified by diffing the boot log against a pre-change baseline.
- **Server** (`moly/tge-server`) — Core, Events, AssetLoader, Scene, Animation (later Network). No Window, no
  Renderer, no Debug. Its own repository declaring this context, not a branch inside the demo; the demo's old
  `InitServer`/`MainLoopServer` scaffolding was deleted. Its schedule collapses to
  `FrameStart[Core Events] → Simulate[Animation Scene]` — no renderer means nothing to pace and nothing to render.

A context is selected by the composition root **passing it directly** to
`gRuntime->Initialize(GetRunContext())`; there is no `--context` flag and no `gSettings.runContext`. **The
module list is unordered** — the runtime sorts it — so a context author cannot silently restate the
AssetLoader-before-Renderer constraint wrongly.

## Runtime rules

The runtime resolves a context before initializing anything, and every failure is a boot failure with a
`gLog.Error`, never a silent degrade:

| Condition | Outcome |
|---|---|
| `Required` dependency absent from the context | **Error, boot fails.** The context definition is wrong — e.g. asking for Debug without Renderer |
| `Optional` dependency absent | Fine. The module loads; the sibling's global stays null |
| Dependency present but `minVersion` / contract unsatisfied | **Error, boot fails** (added by versioning, not step 1) |
| Cycle among `Required`/`Optional` edges | **Error, boot fails**, naming the cycle |
| `Notify` dependency absent | Fine, and no notification is ever delivered |
| Module scheduled but not in the context | Existing `RegisterUpdate` error |
| Module in the context, neither scheduled nor `DeclareNoTick` | Existing `ValidateSchedule` error |

Init order becomes the topological order of `Required` + `Optional` edges; terminate order is its reverse.
Registration order stops carrying meaning, which removes a class of ordering fuse rather than adding one.

## Context traits — designed, NOT shipped

**This section is a design held in reserve, not shipped code.** `EContextTrait` was deliberately not built:
every one of the eleven `headlessMode` guards turned out to be a *presence* question that dependency
declaration already answers, so the enum would have arrived with zero consumers. `SRunContext` carries no
`traits` field today. The design below stands for when a real *property* consumer first appears; the
presence-versus-property line is what decides whether one has.

A context may declare properties that a loaded system consumes when its behaviour genuinely differs. These
are **not** a replacement for `headlessMode`; they are a different mechanism with a narrower job.

```cpp
enum class EContextTrait : uint32_t
{
	None     = 0,
	Headless = 1u << 0   // no display or presentation surface exists in this context
};
```

The trait lives in the context definition, so it is declarative, stated in exactly one place, and every new
context is forced to answer the question. Compare today's `headlessMode`: a CLI flag that leaked into nine
scattered branches across four modules, where the only way to learn a context's shape is to grep for it.

**The line that keeps this from rebuilding what it replaces:**

> **Presence questions are answered by dependency declaration. Property questions are answered by traits.**

A module that tests `Headless` in order to decide whether to talk to the renderer has reintroduced the bug
under a new name — it should declare Renderer `Optional` and check presence. A trait is only for behaviour
that differs *within* a module which is loaded either way: choosing a cache policy, skipping a decode whose
only consumer is a display, picking a log format. **If the answer is "then I do not use module X", it is a
dependency, not a trait.**

Whether a bitfield is warranted for a single trait, or a plain `bool headless` is the honest minimum until a
second one exists, is worth deciding when the first consumer appears rather than now.

## What this deleted

`gSettings.headlessMode` and every guard reading it — in `window/glfw/private/module.cpp` (5),
`renderer/vulkan/private/module.cpp` (3), `animation/private/module.cpp` (1), `debug/private/model_spawner.cpp`
(1), plus the branches in `main.cpp`. Those modules are no longer *loaded but inert*; they are simply absent.
The globals stay null, but the confusing middle state — present, ticking, and unusable — is gone.

**Measured, and the earlier prediction of a net-negative line count was wrong:** the deletion is −98 lines,
the mechanism that replaces it is +312, so the change is **+214** (+394 counting the unit tests). The
prediction had only weighed the removal, not the machinery that makes the removal possible.

## What contexts do not solve — and how step 2 finished it

**Contexts do not force the null check.** Where a module *is* optionality-aware it still rests on
`gRenderer != nullptr` discipline on a global. The mechanism removes the *need* for that check in most modules
by not loading them; it does not make the remaining checks safe. Step 2 did that for the two modules that must
run renderer-absent, Scene and Animation.

For Scene — the hard case at 57 renderer call sites — `CScene::Update` was already close to separable:

```
1. FinalizeSpawn + ReleaseUploadHandle                  renderer
2. m_world.UpdateWorldTransforms()                      pure
3. BVH build                                            pure
   SetSceneBounds / camera frustum / TouchMeshTextures  renderer
4. SyncRenderer()                                       renderer, already its own function
```

**What shipped is NOT the `CSceneRenderLink` wrapper class this document originally proposed.** That was
rejected as an indirection rather than an abstraction — every renderer type still crossed it, and most of its
methods would have had a single call site, i.e. a renamed guard. Instead the renderer-facing work was bundled
into named helpers with one plain guard per region; Animation went from a proposed five-method link class to
two guards. The `Load`/`Save` clusters (~20 and ~15 sites) pull camera, lights and exposure out of the
renderer to serialize them — renderer state living in the scene file, not applicable on a server — and are
guarded the same way.

## Verification

- Default context boots with **zero** validation errors and is behaviour-identical: rendering 87/87, unit
  178/178, all four debug-flag configurations plus clang.
- Server context boots and ticks without crashing from its own executable — the reproduction above was the
  acceptance test, now met.
- Negative controls, each shown to fail loudly: drop a `Required` dependency from a context; introduce a cycle
  among `Required` edges; leave a module scheduled but out of the context.
- Unit tests for the topological sort and cycle detection. Pure logic with a known answer, which is the kind
  of boundary worth unit-testing.

**Known limitation of the cycle diagnostic:** the resolve reports every module it could not place, which is
the cycle *plus everything downstream of it*. The control that made `Window` require `Debug` printed
`Window Renderer Scene Animation Debug` when only the first three formed the loop. Actionable, but naming the
minimal cycle needs a strongly-connected-components pass rather than Kahn's leftovers.

## Open decisions

1. ~~Is `Animation` `Required` or `Optional` on Renderer?~~ **Settled: `Optional`.** A dedicated server needs
   joint transforms for server-authoritative movement and hitboxes, so it must run animation without a
   renderer. The cut is clean — all four of Animation's renderer calls are *output*
   (`SetMeshMaterialFactor`, `SetMeshTextureTransform`, `GetCurrentFrameIndex`, `SubmitMeshDeform`), while
   clip sampling and skeleton evaluation are renderer-free. Four sites behind one boundary, against Scene's 57.
2. ~~Does `--headless` survive as an alias?~~ **Settled: the flag is deleted.** A context is selected instead.
   **No trait enum shipped**: every one of the eleven `headlessMode` guards turned out to be a *presence*
   question, which dependency declaration already answers, so `EContextTrait` would have arrived with zero
   consumers. The traits section above stands as the design for when a real *property* consumer appears — and
   the presence-versus-property line is what decides whether one has.
3. ~~**Should `docs/` stop being gitignored?**~~ **Settled: yes.** These documents are tracked; this file
   reaching a clone is the proof.
4. ~~**How does a consumer author its own run context?**~~ **Answered structurally: the game authors it,
   because the game is the composition root.** Not a mechanism to design — a consequence of the layering. What
   remains is the migration, which blocks the repository split rather than the next step.

## Adjacent, deliberately out of scope: fixed timestep

The engine advances every module by **one variable delta**, clamped once at `MaxFrameDelta{ 0.1f }`
(`runtime.cpp:97`). There is no fixed-step machinery anywhere.

Multiplayer generally wants deterministic fixed-rate simulation — a server ticking at 20–60 Hz independent of
any render, and a client running fixed-step prediction alongside a variable-rate render. That changes the
`Simulate` phase's contract, and the server context is where it will first bite.

**It is not a context problem and must not be folded into this arc.** Module loading and the simulation
timing model are separate concerns; combining them would make both changes harder to review and to revert.
Flagged here only so the arc does not accidentally foreclose it — nothing in the context design assumes one
`Simulate` per frame, and a fixed-step driver would sit above the schedule rather than inside it.

Likewise, where a Network module ticks (receive before `Simulate`, send after — possibly in existing phases,
possibly needing its own) is worth deciding when the module is real, not now. The phase enum is designed to
grow.
