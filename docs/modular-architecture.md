# Modular architecture — the direction

Tge is the proving ground for a modularity model intended for CRYENGINE: **a small core plus a pool of
independently versioned modules, where nothing is mandatory except Core.** A consumer links only what it
needs and updates each module on its own clock, instead of taking a whole-engine release to pick up one fix.

This document is the north star and the sequencing. The concrete work in flight is in
[run-contexts.md](run-contexts.md).

## The destination: there is no engine

**"The Tge engine" is a scaffold, and the goal is for it to stop existing as a thing.** What remains is a
pool of individual, standalone, self-sufficient features — Scene, Renderer, Asset Loader, Animation,
Window — each in **its own repository**, and an app that submodules or links whichever of them it wants.
There is no engine release, no engine repository and no engine version, because there is no engine: an app's
dependency list *is* its engine, and no two apps need the same one.

**This repository becomes solely `Tge-Demo`** — one app among the possible consumers, which happens to be
the one that exercises the pool most completely. It is not the engine that the modules were carved out of;
it is what is left once they have all left.

**Debug is the exception, and it is not in the pool.** It is where functionality from several modules
converges to be inspected, which only means anything somewhere that already links them all — an application.
So Debug is not a module repository in waiting; it is an app's own debugging layer. Packaging it would be
preparing it for an extraction that will never happen. It stays a library of this tree, linked by the
consumers that compose one: the demo, and the rendering-test harness, which is a composition root too.

That makes a rule for every other module: **nothing in the pool may depend on Debug.** An edge from a module
into the app layer cannot survive the split, because a standalone module repository has no app to reach into.
Window held exactly one such edge — it registered its own console commands — and severing it is what made
Window packageable. The Renderer held the same edge at eighteen commands rather than two; it is severed the
same way, and no module in the pool names Debug any more.

### A repository is not a library

"Its own repository" says nothing about how many libraries live inside it, and this tree has already settled
the distinction three times. **tge-core** is a repository of focused libraries — `TgeLogging`, `TgeMemory`,
`TgeMath`, `TgeModule`, `TgeLifecycle` — under an umbrella target, so a consumer chooses its granularity
*inside* the repository rather than by choosing between repositories. The **Asset Loader** ships `public/` and
`private/` as two targets from one repository. **Window** ships its interface and its GLFW backend the same
way, from one export set.

So a module repository holds the whole module: its interface, its implementation, any alternative backends,
the tools that build its own inputs, and its tests. An interface given a repository of its own has no
consumers but its siblings, which is the ceremony the pool exists to remove.

**The Renderer is where this is worth stating outright**, because it is the module whose interface and backend
look most separable. `tge-renderer` is `public/`, `vulkan/private/`, the shaders, the shader compiler that
builds them, and the rendering tests: **a consumer pins one release and has everything it needs to put pixels
on screen.** A second backend arrives as a sibling target in that repository, exactly as `TgeWindowGLFW` sits
beside `TgeWindowPublic` — not as a further repository.

That test also settles which way a shared-looking tool moves. The shader compiler has a generic half — shaderc
driver, include resolver, content cache, SPIR-V validator — and it stays in the Renderer's repository anyway,
expressed as targets *within* it. Lifting it into Core would let a consumer pin a renderer release and still
be unable to build its shaders, which is the thing pinning exists to prevent. **A module's genericness is a
reason to structure it, not a reason to move it out.**

**Where the separation is real it is a COMPONENT, not a repository.** Scene, Animation and a dedicated server
need the Renderer's interface and must never pull Vulkan; the demo needs a renderer that draws. That is
`find_package(TgeRenderer)` against `find_package(TgeRenderer COMPONENTS Vulkan)` — one repository, two export
sets, the consumer stating which half it wants. tge-core already does this for its test harness
(`COMPONENTS Testing`).

**That is also what keeps the public interface single for a multi-platform title.** A title shipping on
several platforms needs several backends, and **there must be no overlap on the public interface** — one
`IRenderer`, one archive, one header set, however many backends are in play. A component split gives that by
construction: one pinned version of the repository means one `Tge::RendererPublic` no matter which components
are requested. Making each backend its own separately pinnable package would not: two backends pinned at
different versions install two copies of the headers, and a backend compiled against one `IRenderer` layout
would link against a header describing another — a runtime failure, not a link error, and one the boot-time
`ModuleContractVersion` check does not cover because the interface is a header. The case that settles it is
runtime backend selection in a *single* binary (a Windows build offering both D3D12 and Vulkan): one process,
two backends, and they must share one interface.

### What a repository may depend on

**Standalone is the aim, not an absolute.** A module that resolves against Core alone is the better outcome
and two already do — Window and the Asset Loader. Where it is not achievable, a dependency on
another module in the pool is legitimate rather than a defect to design away.

**What crosses must be another module's public surface.** A module may name a sibling's public library and
nothing else; reaching an implementation is what the rule forbids, because that is the edge a consumer cannot
see, cannot version against, and cannot substitute. Core stays the one universal dependency — everything
takes it — and every other edge is a stated contract between two systems. **A consumer pulling Scene accepts
Scene's dependency set**, the same way it accepts any library's.

The pool satisfies this today, measured rather than intended: **no module links another module's private
target.** Every cross-module edge resolves to a `*Public` library, and only composition roots — the demo, the
rendering-test harness — link implementations.

| repository | contracts it declares beyond Core |
|---|---|
| `tge-window`, `tge-asset-loader` | none |
| `tge-renderer` | `Tge::AssetLoaderPublic` — `IRenderer::CreateMeshes` takes an `SModelData` |
| `tge-scene` | `Tge::AssetLoaderPublic`, `Tge::RendererPublic` |
| `tge-animation` | `Tge::ScenePublic`, `Tge::RendererPublic`, `Tge::AssetLoaderPublic` |

That the composing modules declare more than the leaf ones is the shape of the pool, not a violation of it.
Scene's job is to drive a renderer, so an edge to the renderer's public interface is Scene describing what it
is, and a consumer that wants a scene system wants that edge.

**The edges must also be acyclic, and one of them was not.** Scene named `Tge::AnimationPublic` too, because
it called the animation module directly when a model spawned or was removed — while Animation names
`Tge::ScenePublic`. Two packages whose configs `find_dependency` each other cannot be resolved, so the pair
was unpackageable as long as that call existed. Scene now *announces* `SModelSpawned` / `SModelRemoved` and
Animation subscribes, which deletes the edge rather than working around it: a module that announces what
happened to it does not need to know who is listening. A cycle between two modules is a signal that one of
them is reaching where it should be publishing.

**This does not make relocating a shared type pointless, only optional.** A type two modules embed by value
still cannot version independently of either — the reason Geometry and Material sit in Core — so moving one
remains the right call when a contract turns out to be vocabulary rather than collaboration. It is no longer
a precondition for a repository to stand up.

That end state is what makes the requirements below non-negotiable rather than tidy, because a repository
that must stand on its own has to carry everything it needs to be built, run and trusted:

- **Each repository links its own external requirements.** The Asset Loader was the first that did: draco,
  KTX, libwebp, meshoptimizer, tinygltf and json are submodules of *its* repository, and a parent that
  already declares one of them wins through a guard rather than by being reached into. The Renderer now does
  the same with VMA, stb, ImGui, FreeType and KTX. What is left going through the *engine's* `external/` is
  what the app layer itself needs — googletest, benchmark, tracy, json — plus FreeType and KTX, which stay
  declared here so one target serves every guard. In the destination each module's set is its own to vendor
  or resolve, and nothing sits above it to provide them. **Two repositories needing the same library is normal, and not duplication to
  design away.** The Asset Loader and the Renderer both use libktx and both must declare it: the loader
  transcodes with it, while the renderer opens KTX files itself for the environment and cubemap path. Taking
  it transitively from the Asset Loader would be an undeclared dependency of the kind this arc keeps
  uncovering — the loader links `KTX::ktx` PRIVATE, so its public contract promises nothing about supplying
  it, and its `find_dependency(Ktx)` is emitted only when *it* vendored the library. Both declare it,
  whichever resolves first defines the target, and the other's guard no-ops.
- **Each repository carries its own proof.** A module repository is three layers: Core underneath, the
  feature in the middle, and the tests that prove it on top — a minimal runnable environment, or a
  unit/feature testing layer sophisticated enough to stand in for one. This does *not* mean testing in
  isolation: a module's tests may link **real sibling modules** to give the feature something to run in
  (Animation's want a renderer and a scene), plus a **shared harness common to every module's tests**. What
  makes it self-sufficient is that this set is *stated and owned by the module's own repository* rather
  than implied by whatever tree it happens to sit in. A module whose only proof lives in a demo app
  elsewhere is a fragment with a remote test suite, and the dev in step 2 of the workflow below cannot
  validate a fix without cloning the thing they were trying not to depend on.
- **Each repository is consumable as source or as a binary.** Consumers submodule the source and compile it
  themselves, or link prebuilt libraries where source cannot be exposed. Binary distribution is the
  demanding case and it is what the next section is about.

## The production workflow this is all for

Everything below serves one target experience, and design decisions should be judged against it:

1. A game lives in **its own repository** and links **released static libraries** — one per module, versioned
   independently. Shared libraries stay available for consumers who need them, but binaries are the default.
2. When a module misbehaves, the game **submodules that module's source**, builds it alongside the game, and
   fixes the bug in place — no waiting on an upstream release, no vendored fork.
3. The fix is **committed back to the module's own repository**.
4. That module cuts a release, and the game **drops the source submodule and returns to the binary** whenever
   it chooses to move.

The property that makes this humane is that **step 2 is never a fork and never a blocker**. A dev is one
submodule away from source-level control and one release away from handing it back.

### What that forces on the design

**Binary-by-default is the demanding half.** A source-only distribution forgives almost everything here; the
moment a prebuilt `.a` meets a game built separately, four things stop being style preferences.

**1. A module's public headers must compile to one layout, whatever the consumer's build flags.** A public
struct whose size depends on `TGE_DEBUG_ENABLED` links without complaint against a differently-configured
game and then reads every member at the wrong offset — silent corruption, no diagnostic. Tge's existing rule
(*never `#ifdef` a function signature*; declare unconditionally, gate only the body) is exactly the right
rule and mostly holds: `renderer.hpp` uses unconditional signatures with no-op defaults, and `SMeshInfo`
carries its debug `string_view`s in every configuration. The exception is
`asset_loader/public/asset_loader/model_data.hpp`, where `SMaterialData::materialName` and `SMeshData::name`
are `#ifdef`'d members — two sites, both layout-changing. **Extend the rule from signatures to layout, and
make it a checked property rather than a convention.** It also argues for keeping the flag count low: every
ABI-affecting flag multiplies the release matrix.

**2. The context tables belong to the game, because the game is the composition root** — see the section
below, which is the structural reason. The distribution model corroborates it from the other side: a static
library contributes only the objects something references, so **whatever names a module is what links it**,
which makes a run context a *link-time* concept as much as a load-time one. Engine-private tables naming all
eight modules (as step 1 shipped, correctly, for two engine-owned contexts) would make every game link every
module, Vulkan and GLFW included.

**3. Version constraints must be enforced at load, not documented.** In a source build a mismatched pair is a
compile error. In a binary build it links cleanly and fails later, far from the cause. This now holds:
`SDependency` carries a `minVersion`, and `CModuleGraph::Resolve` enforces both a semver-compatible dependency
constraint and a module-contract-version check — the same resolve that already fails a boot loudly for a
missing `Required` module.

**4. The binary and source paths must present the identical build interface.** `find_package` and
`add_subdirectory` have to yield the same namespaced target with the same usage requirements, include
directories and compile definitions. If swapping between them is more than a one-line change in the game's
build, step 2 of the workflow stops being cheap and the whole loop breaks. This holds for tge-core and every
module in the pool today: all install and export namespaced `Tge::` targets, and
`tests/packaging/verify_package_interface.sh` builds a consumer against each path and diffs the two
interfaces. Extending it across the module pool is mechanical.

One consequence for sequencing: **step 3 (tests as contexts) is load-bearing, not a nicety.** A dev who
submodules a module to fix it needs to run that module's own tests without standing up the game — which is
precisely what "every module carries its own proof" buys.

## The composition root: where everything is molded together is the game

In the destination there is no monolithic Tge to link. There is a pool of module libraries, a vocabulary
library they all compile against, and **a game that composes them** — which module set, which schedule, which
settings. That composition is policy, and policy is the game's.

`code/runtime/` was two things wearing one name, and it has since **split**: the mechanism (the dependency
sort, the lifecycle, the phase dispatch) moved to tge-core, and the policy (the context tables) moved to each
composition root. The mechanism ships from tge-core, which is the one repository every consumer takes — no
game should reimplement a dependency sort and a phase loop.

What is left on the policy side:

- **The frame loop is already game-side.** `code/main.cpp`'s `MainLoop()` owns the `while`, the delta
  and the call in; `CRuntime::Update` only dispatches the phases in order.
- **`gSettings` is still game configuration living in an engine library** (`code/tge/public/tge/globals.cpp`).
  Asset paths, window dimensions, the HDRI. It has a definition TU of its own now, but it has not travelled to
  the game.

### What cannot move, and why it is a hard constraint

**`EFramePhase` must stay engine vocabulary.** Modules branch on it — `CRenderer::Update` switches on `phase`
— so a module shipped as a binary was compiled against one specific phase enum. If a game could define its
own, module binaries could not be produced at all. It lives in tge-core's `tge/module/frame_phase.hpp`,
outside any module and outside the game. The same reasoning covers `IModule`, `SDependency` and
`SRunContext`: the *types* are contract, only the *tables* are policy.

### Lifecycle and phases moved to tge-core — SHIPPED

The proposal's rule is **nothing is mandatory except Core**, and module lifecycle plus phase dispatch are
needed identically by every consumer, which by that definition makes them Core. They now live there, as
`TgeModule` (the contract) and `TgeLifecycle` (the driver); `code/runtime/` is gone.

The objection this had to clear was that Core must not become a dumping ground. It does not apply, because
**tge-core is already a repository of focused libraries rather than one library**: `TgeLogging`, `TgeMemory`,
`TgeMath`, `TgeThreading`, `TgeIO` and `TgeInit` are separate static libraries over a `TgeBase` interface, and
`TgeCore` is an INTERFACE target that only links the set. A consumer wanting arithmetic links `TgeMath`; the
umbrella is a convenience, not a constraint.

So lifecycle arrived as **sibling targets in that repository**, not as a widening of Core-the-library — the
repository's established pattern rather than a new concept. That makes the rule literally true for the first
time: Core-the-*repository* is the single mandatory dependency, and a consumer chooses its granularity inside
it. A module repository standing alone now depends on exactly one thing instead of on Core plus a separate
vocabulary library.

**Moved:** the contract types (`IModule`, `IModuleId`, `SDependency`, `EFramePhase`, `SRunContext`) as public
headers under `tge/module/`; `CModuleGraph` and `CRuntime`'s lifecycle and phase dispatch as `TgeLifecycle`'s
implementation. `TgeProfiling` went with them — the phase plots are part of the dispatch, and Core cannot link
an engine target. `TgeCore` carries the contract but **not** the driver, so declaring a dependency still links
nothing that can drive a module.
**Did not move:** `SSettings` and the context tables, which travel the other way to the game, and each
module's own `CModule` — Core's own travelled with Core, since Core is the module it wraps.

**The cost, which is real and accepted rather than absent:** the module contract begins versioning with
tge-core, so any change to `IModule` or `EFramePhase` is a Core release that every module rebuilds against.
That is correct — it genuinely is a breaking contract change — but it places the concept most likely to keep
evolving inside the library one most wants frozen. This document elsewhere says the phase enum is designed to
grow; under this move, growing it ripples through every module repository.

The common case is gentler than that sounds. **Appending a phase is binary-safe for module code**: modules
receive `phase` by value and branch on it, and only `FramePhaseCount` array sizing breaks, which lives inside
the lifecycle target and rebuilds with it. The residual hazard was behavioural, not ABI — the renderer's
module used `else` as "Render", so a build handed an unknown phase would have recorded a whole frame for it.
Its dispatch is now explicit and an unknown phase is a no-op.

## Where Tge already stands

More of the skeleton exists than one would expect:

- **tge-core is the carve-out already** — math, memory, logging, threading, init, profiling, in its own
  repository with its own remote, and structured as focused libraries under an umbrella target rather than as
  one library. That is the proposed Core's primitives; module lifecycle now sits beside them as a sibling.
- **Modules declare dependencies by kind** (`Required`/`Optional`/`Notify`) and the runtime derives
  initialization order from the graph rather than from a hand-maintained registration list.
- **Interface and implementation are already split** — `tge-renderer`’s `public/` versus
  `vulkan/private/`, and the same shape in every other module.
- **The frame schedule is already separate from registration** (`EFramePhase`), so *what loads* and *what
  ticks* are independent questions with independent answers.

The two things that were missing — **optional dependency** and **versioning** — have both shipped. What
remains is breadth (extending packaging across the pool), **self-sufficiency** (each module owning the
external dependencies it currently reaches through the shared `external/`, and carrying its own way to be
run and proved), and the repository split itself; see Sequencing.

Self-sufficiency is the part with no scaffolding yet, and it is easy to under-read because everything
compiles today. Every module's tests live in this repository's `tests/`, and the rendering suite needs a
window, a device and the demo's assets — so a module extracted tomorrow would arrive in its new repository
with no way to prove itself. The **shared test harness** the model expects every module's tests to sit on
does not exist as a consumable thing either: `tests/utils/` and `tests/rendering/utils/` are the right
material, but they are this tree's private helpers rather than something an extracted repository could
depend on. Both are per-module costs the packaging work does not touch.

## The gap that mattered first — closed

The proposal's rule — nothing mandatory except Core — implies that any module able to run without a sibling
must be *designed* for that sibling's absence, and originally nothing in Tge was. `Renderer::gRenderer` was
dereferenced **268 times outside the renderer** with no null check (debug 196, scene 57, tge_demo 11,
animation 4). The one place it was not masked was `--headless`, where the renderer left `gRenderer == nullptr`
and the process segfaulted during `Debug` initialization, before the first frame.

**That crash was the modularity problem in miniature**, not an unrelated bug: Scene assumed Renderer the way
CryEntitySystem assumes CryRenderer. Step 2 closed it — Scene and Animation now run with the renderer absent,
the server context boots and ticks them, and the `--headless` flag is gone. The method generalised, which is
what de-risked the proposal.

## Sequencing

**1 — Optional dependency and run contexts. — SHIPPED.** The mechanism. A context names a set of modules and
a schedule; modules declare which siblings they require, which they merely use, and which they only want
notified about. Design: [run-contexts.md](run-contexts.md).

**2 — Scene and Animation as the proof. — SHIPPED.** Two real modules that run correctly with a sibling
absent, with their renderer-facing work concentrated behind a single boundary rather than sprinkled across
call sites. This is where the technical risk was actually retired, and it fixed the headless crash as a side
effect rather than as the goal.

Animation is the easy half and a useful warm-up: all four of its renderer calls are *output*
(`SetMeshMaterialFactor`, `SetMeshTextureTransform`, `GetCurrentFrameIndex`, `SubmitMeshDeform`), while clip
sampling and skeleton evaluation are already renderer-free. Scene is the hard half at 57 sites.

**This step is a shipping prerequisite, not a demonstration** — see the dedicated server below.

**3 — Tests as contexts. — STARTED.** The proposal's middle section — every module carries its own proof, and
its tests declare the sibling modules they need to stand up an environment. That declaration *is* a context,
so the existing 87 rendering and 178 unit tests become the mechanism's second real consumer. This matters: a
context system with one consumer is speculative infrastructure. `tests/packaging/` is the first sliver — a
consumer that composes Core + Events into a context and boots them from installed packages — but a proper
per-module harness is unbuilt, and tge-core still has no test harness of its own.

### The consumer that justifies it

A **network module and a dedicated server application** are planned, for basic multiplayer. That answers the
question this arc would otherwise have to keep answering: whether a second run context has a real customer, or
whether the mechanism is infrastructure for a mode nobody runs.

It is real. The server context becomes a product target rather than the smallest useful forcing function,
which raises the bar on step 2 — "Scene runs without a renderer" stops being proof that the method generalises
and becomes a prerequisite for something that ships. The dedicated server is its own executable declaring the
server context, and the demo's vestigial `InitServer`/`MainLoopServer` branch should be deleted rather than
grown into it.

Both have happened, and the server has since gone further: it is **its own repository** (`moly/tge-server`),
consuming the module packages and composing nothing of this tree. It is not submoduled back — this repository
is Tge-Demo, and the server is a second product rather than a part of the first.

Network itself needs no special accommodation — `Required{Core, Events}`, nothing display-side — which makes
it the mechanism's best validation: if the design is right, adding it should require touching no existing
module. It lands after step 1, and should not be allowed to drive the design before it exists beyond
confirming it slots in.

**Fixed-rate simulation is the one genuinely new question it raises**, and it belongs outside this arc: the
engine has a single variable delta and no fixed-step machinery, while multiplayer generally wants
deterministic ticking. That is a simulation-timing change, not a module-loading one. See `run-contexts.md`.

**4 — Versioning, then repository split. — VERSIONING SHIPPED; THE POOL IS SPLIT OUT.** Stated
per-module contracts and expressible constraints ("Scene 2.5 needs Renderer ≥ 4.1"), enforced at load rather
than documented. This split into two halves. The load-time half shipped first: `SVersion`, a semver-compatible
`Satisfies`, `SDependency::minVersion`, a `ModuleContractVersion`, and both checks in `CModuleGraph::Resolve`.
The build-interface half shipped next: tge-core, Events and the Asset Loader's public library install and
export namespaced packages, so a module binary built against a different contract than the runtime is now
*rejected at boot* — a mismatch that was not physically constructible while everything built from one tree.
No module declares a `minVersion` yet (deliberate: one tree, one version, nothing a reader could act on),
and there is no `modules.lock` (nothing is released to pin). **Every module in the pool is packaged now,
implementation included, and every one is its own repository** — the Asset Loader, the Renderer, the Animation
and the Scene. This repository is Tge-Demo composing them.

**The three that were left settled differently, and two of them do not become repositories.**

- **Events was hoisted into tge-core**, not extracted. It depends on nothing but the module contract while
  nearly every module depends on it, and it ships no payload types — the modules own those. A subsystem with
  that shape is core rather than a peer, so a repository for it would have been a package every consumer
  had to find in order to reach something Core already gave them. It stays outside the `TgeCore` umbrella:
  `QueueEmit`'s backlog is drained by a frame phase, so a consumer that runs no frame loop must ask.
- **Debug stays the app layer.** It is the demo's own debugging tooling, and packaging it would prepare it
  for an extraction that is not going to happen. Only composition roots name `Tge::Debug`, which is what
  that position predicts.
- **Window is the last extraction**, and it is the only one whose module gate runs no unit suite: its
  behaviour depends on wall-clock timing against the compositor, so a fast-frame harness observes the
  correct result even when the bug is present. Its package closure probe stands in for one.

Packaging them is **not uniformly mechanical**, which the earlier "package the other seven (mechanical)"
framing hid. Two things shape the order and the cost.

*Ordering is forced by the public targets.* `Tge::AssetLoaderPublic` depends on Core alone, and Renderer,
Scene and Animation all reach it transitively (Renderer → AssetLoader; Scene → AssetLoader + Renderer;
Animation → all three), so nothing downstream can be packaged before it. That is why the Asset Loader's
public library went next, and why Window — with Debug out of the pool — is the one that follows. The Renderer
in turn gates Scene and Animation, and it is the interface half that gates them: Scene and Animation link
`Tge::RendererPublic` and never the backend. That half is packaged now, and it needed no
`find_dependency(TgeWindow)` — the public headers reach no window; only the Vulkan backend links
`Tge::WindowPublic`.

*Cost is shaped by each implementation's third-party links.* Animation links only `Tge::` targets and was the
true repeat of the Events pattern; Scene proved to have one vendored library after all — its serializer's
nlohmann/json, reached through the shared `external/` by an absolute path. It resolves that through its own
`TGE_SCENE_EXTERNAL_ROOT` now, under a copy the module owns and no consumer overrides. Both are packaged,
implementation included. Window adds `glfw`, resolvable as a system
package — but only the system branch is exportable, because a FetchContent'd glfw is a real target that
belongs to no export set. The Asset Loader and the Renderer were the hard ones: their **private** targets link vendored
libraries, and a module repository cannot stand alone until that half is packaged too. Each vendored library
installs its own package config and the module's config `find_dependency`s exactly the ones it built.

The Renderer's backend was the last of them, and it is where the component split stopped being a design
sketch. It ships as `find_package(TgeRenderer COMPONENTS Vulkan)` against the interface's plain
`find_package(TgeRenderer)` — one repository, two export sets, the consumer stating which half it wants — and
**the backend's half loads only when the component is requested.** Loading it merely because it is installed
would make a graphics API a configure-time requirement of every module that takes the interface from the same
prefix, which is the property the split exists to protect and which no amount of link-time care recovers.

Its four blockers all yielded to the same reading: what a module reaches for, it must declare. `freetype` and
`KTX::ktx` were added `EXCLUDE_FROM_ALL`, so CMake ignored their own install rules; the module adds them
itself now and installs only what it built. `Tge::Utils` had no install and published an include directory
covering all of `code/`, and it turned out to have one real consumer, so it was absorbed rather than packaged.
Fifteen paths named `${CMAKE_SOURCE_DIR}`; three were redundant against the targets beside them and the rest
resolve through the module's own external root. The shaders reached the public `.inc` files by walking up out
of the shader directory, which means nothing against an install prefix — the search path that already carried
them was there the whole time, and flattening the eight directives left all 82 SPIR-V outputs byte-identical.

A packaged backend also has to be *usable*, which the library alone is not: the install carries the compiled
SPIR-V, the shader sources and the compiler that builds them, and the config publishes where they landed.

**Packaging is necessary but not sufficient for the split.** A packaged module still reaches its external
dependencies through the shared `external/` and still has its tests in this repository; both have to become
the module's own before its repository can stand up. Packaging is what makes a module *consumable*;
self-sufficiency is what makes it *extractable*, and only the second one ends with an app that no longer
needs this tree.

The Asset Loader made that second crossing first: it is `moly/tge-asset-loader`, submoduled here at
`external/tge-asset-loader`, owning its externals, its nine test files and its own packaging gate — which
runs in its own clone with nothing of this tree on any path. What it cost that packaging alone did not is
the shape the ones after it repeated: the module's tests had to leave a single flat engine test binary, and
the shared build scaffolding it includes had to become a copy it owns rather than a reach up two directories.

**The Renderer has now crossed too** — `moly/tge-renderer`, submoduled at `external/tge-renderer`, entered
through one `add_subdirectory` where this tree previously named five of its subdirectories directly. Being
the largest module it added two costs the loader did not pay, and both are worth expecting again:

- **A suite had to be split before it could travel, not a link removed.** `TgeRenderingTests` proves the
  renderer and went with it; `TgeCompositionTests` needs a scene graph, clips and the command table, so it
  stayed with the app that composes those. The firewall that made the split decidable is a *link* property —
  the renderer half cannot even include the app-layer modules — and it now spans two repositories.
- **A library consumed as SOURCE rather than as a target follows the module, and its consumers must follow
  it there.** Debug compiles ImGui translation units of its own and takes the external root the module
  publishes back instead of naming a copy here — not because the Renderer owns the ABI (it dropped ImGui
  entirely in 2026-08), but because a second checkout would be a second pin nothing arbitrates. stb shows the
  other end of the same rule: the Renderer keeps it behind a private include directory no consumer sees, so
  it follows the module and stops there. A library consumed as a *target* needs none of this: FreeType and
  KTX stay declared in both places and the guard picks one. The distinction is who compiles the source.

Splitting `code/runtime/` along the mechanism/policy line was a prerequisite for this rather than a separate
ambition, and it is done: the lifecycle machinery is a tge-core sibling target and the context tables are
authored by each composition root. `gSettings` moving to the game is what remains of that split.

## Known hard parts

- **Global-pointer coupling.** `Renderer::gRenderer` is an `extern` in a public header — a compile-time hard
  link. A per-module-versioned pool wants the interface to be the contract and the instance to be discovered.
  The `IModuleId` tag pattern is halfway there already.
- **Versioning implies interface stability**, which implies a deliberate decision about what each module's
  public surface promises. Under binary distribution the promise covers **layout**, not just names and
  signatures — see the production workflow above. Tge has begun to make it: the module contract is versioned
  and enforced at load, and the package boundary now carries the layout-affecting build flags as target usage
  requirements so a consumer cannot compile a public header down a different branch than the binary. What each
  module's public surface promises beyond that is still per-module and largely implicit — the one named
  layout violation is `asset_loader/public/asset_loader/model_data.hpp`, whose `#ifdef`'d members change a
  public struct's size, and it becomes live the moment asset_loader is packaged.
- **Linker dead-code elimination** already forces explicit module registration; per-module libraries make that
  constraint more visible, not less. It is also the mechanism that makes a consumer-authored run context
  decide what gets linked, so the same constraint that looks like an annoyance today is what delivers "link
  only what you need".
