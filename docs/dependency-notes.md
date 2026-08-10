# Dependency notes — tge-core

Split out of the workspace-level `TGE_DEPENDENCY_HANDOFF.md` on 2026-08-06, so each finding sits in the
repository whose session will act on it. Everything here was measured on Thomas's machine, not inferred.
The cross-cutting index lives in `tge-demo/docs/dependency-notes.md`.

## OPEN 2026-08-10 — a vocabulary layout mismatch between two modules is undetectable, and needs its own session

**The question that framed it (Thomas):** module A is built against a Core whose `struct S { float f1; float
f2; }`, module B against a Core where those two fields were swapped. Both are 8 bytes, both 4-aligned. An
application linking A and B has no way to notice.

**Why it is silent rather than merely undetected.** The vocabulary targets are `INTERFACE`, so every consumer
compiles its own copy of the layout. Inline functions and templates emit weak symbols; the linker keeps one
definition and discards the duplicates, which is what it is supposed to do. The result is one version's code
operating on another version's layout. The linker is not failing to catch this — it is actively what makes it
invisible.

**Nothing currently in place covers it, and each for a different reason:**

- The `sizeof`/`offsetof` asserts beside each vocabulary type (`6647d44`) stop the **author**: swapping the
  fields moves `offsetof` and fails the build here, forcing the change to be made deliberately. But once a
  Core ships with corrected numbers, each module compiles cleanly against whichever header it has. **They make
  the change deliberate; they do not make the mismatch detectable.** A same-size reorder is exactly the case.
- `ModuleContractVersion` describes `IModule`/`EFramePhase`, is hand-maintained, and stays `1` when a
  vocabulary field moves.
- The pin gate proves every declarer of Core agrees, but it works only where one gate can see the whole graph.
  **Under released binaries there is no such vantage point** — which is why this is a real problem rather than
  a theoretical one, and why it is not urgent yet.

**The mechanism to build on already exists here, and its comment already states the reason it works.**
`CModuleId`'s constructor takes `contractVersion` as a *default argument*:

> *"contractVersion is a default argument so it is evaluated in the MODULE's translation unit; an inline
> virtual returning the constant would be deduped by the linker and every module would report one value."*

A default argument is evaluated at the call site, inside each module's own TU, against whichever Core header
that module saw. That is precisely the property needed, and the COMDAT hazard that defeats the obvious
alternative is already understood there.

**The shape of the fix:** a second default argument carrying a digest computed from the layout itself rather
than from a maintained number —

```cpp
constexpr uint64_t VocabularyLayoutDigest()   // FNV-1a over sizeof + every offsetof
```

— baked per module the same way, and compared in `CModuleGraph::Resolve` beside the contract check it already
performs at `module_graph.cpp:38`. A swap moves the `offsetof` values, so the digest changes with nobody
touching it, and the boot fails naming both modules.

**Placement is the design question for that session**, and it is a genuine trade rather than a detail:

| | catches a same-size swap | when |
|---|---|---|
| `static_assert` (shipped) | no — author only | compile, here |
| inline namespace keyed on the release **version** | only if someone bumped it | link |
| inline namespace keyed on the **digest** | yes, automatically | link |
| digest in `CModuleId` | yes, automatically | boot |

**A version-keyed inline namespace is the trap to avoid**: a swap that ships without a version bump keeps the
same mangled names and stays silent, so it inherits exactly the weakness of a hand-maintained number. The
digest is the load-bearing half; the namespace is a placement that buys link-time over boot-time and costs a
build-system step, since a namespace name must be an identifier at preprocessing time. `CModuleId` needs no
codegen. nlohmann/json is the reference implementation of the namespace form and is vendored in this graph.

**Residual fuse, to state in whatever ships:** the digest enumerates types and fields, so a newly *added*
type is uncovered until someone adds it — the same list-as-fuse problem the asserts have. A reorder of
existing fields is covered for free, which is the case that prompted this.

## RESOLVED 2026-08-07 — the packaging gate arrived

`tests/packaging/verify_package_interface.sh`, 9 steps and 15 checks, modelled on tge-scene's. It installs
googletest, builds and installs Core out of tree with the harness and both suites on, runs both suites,
consumes the result from packages alone, and proves the `Testing` component is requested rather than
inherited. It re-measures the counts recorded below: 76 and 15.

**Two things this module needed that no sibling gate has.** googletest gets a prefix of its OWN rather than
sharing Core's, so every consuming step runs against Core's prefix alone and "no googletest" is a property of
the search path. Measured rather than assumed — a shared prefix with `lib/cmake/GTest` deleted still
satisfies `find_dependency(GTest)` through CMake's `FindGTest` module, so the mutilated-prefix control every
sibling uses would have passed here for free. And step 9 installs Core a second time with
`TGE_CORE_TESTING=OFF` to hold `install.cmake` to its claim that the two installs publish a byte-identical
package. They do.

The `COMPONENTS Testing` bug fixed in s147 now has a standing regression rather than a memory of one: steps 5
and 6 consume a Core installed *with* the harness from a path carrying no googletest, which is exactly the
shape that broke.

Control: six mutations of the installed prefix, each turning red the step that covers it — the s147 config
restored to loading unconditionally, `tge_module_package.cmake` deleted, the generated `tge/config.hpp`
deleted, glm deleted, and the config's contract-version scrape moved to 99.

## RESOLVED 2026-08-06 — the two benchmarks arrived, and the broken one was repaired

`benchmarks/threading/bench_parallel_for.cpp` and `benchmarks/events/bench_event_system.cpp` measure core
mechanisms and link only `Tge::Core` / `Tge::Events`, so they now live here rather than in the demo.

**No `benchmark` submodule was added, and that was the wrong prescription.** This repository already declines
to vendor googletest — `TGE_CORE_BUILD_TESTS` is off by default precisely so whoever supplies a harness turns
it on. `TGE_CORE_BUILD_BENCHMARKS` follows that rule exactly: `benchmarks/CMakeLists.txt` resolves Google
Benchmark through the same `if(NOT TARGET …) find_package(…)` handoff the unit suite uses for GTest, and the
consuming tree supplies its own checkout. **When adding a dependency to a repository, check first whether an
existing dependency of the same KIND is already handled a particular way** — core's answer for test harnesses
was already written down, and a benchmark harness is the same kind of thing.

The repair: `EEvent::PipelinesSettled` became a locally-defined `SBenchEvent` driven through the templated
`Subscribe<T>` / `Emit<T>` / `QueueEmit<T>` API. Defining the event type in the benchmark is not a shortcut —
it is what the unit suite already does, because **core sits below every module and may not name a module's
event**.

Control: the pre-fix source was restored over the repaired one and shown to fail with exactly the 13
`'EEvent' has not been declared` errors recorded here, then to build clean again once restored.

## RESOLVED by this file — `docs/` was gitignored here

`.gitignore` listed `docs/` under *"Session documentation (project-specific, not for repo)"* alongside
`tasks/`. That blocked moving `modular-architecture.md` (the module pool's charter) and `run-contexts.md`
(this module's own mechanism: `IModule`, `EFramePhase`, `SRunContext`, `CModuleGraph`) into the one
repository every consumer takes.

The rule is now narrowed to `tasks/` alone, so real documentation can live here. **Both documents now live in
this directory** (moved 2026-08-10); they cross-reference each other by relative link and moved together, so
nothing had to be rewritten, and no repository referenced either of them from outside.

## The trap this repository's test layout exposed

A module added `EXCLUDE_FROM_ALL` has its **test binaries excluded too**, and the consuming tree had undone
that for the one suite that existed *by name*. When `TgeCoreThreadingTests` arrived as a second binary it
configured, built nothing, and reported nothing — **which is indistinguishable from a passing gate.**

Anything adding a further test binary to this module hits the same thing. `tge-demo/cmake/external_libs.cmake`
loops over this repo's suites for exactly this reason.

## Done 2026-08-06 — kept because the reasoning still applies

Took six units and the threading suite from the demo: math helpers, the command parser and registry, the log
listener, the module-graph resolver, and `CJobGroup`/`ParallelFor`. The threading tests are a **second
binary** on purpose — they need `Tge::Initialize` where every unit beside them is settled from plain values.

Counts as of 2026-08-06, re-measured across four configs: `TgeCoreUnitTests` **76** (Debug/Release/nodebug)
and **72** in a `TGE_ENABLE_LOGGING=OFF` build, which drops the four log-listener tests because
`test_log_listener.cpp` exercises the log system itself. `TgeCoreThreadingTests` **15** in every config.

## Own dependencies

Modest: glm (75 MiB of history, compiled whole) and rpmalloc (3 MiB).

## OPEN 2026-08-07 — Core is the mechanism the family's edge removal will run through

Stated intent (Thomas, 2026-08-07): **no `tge-*` repository sits above or below another. Core is the sole
exception — the one repository every other builds on.**
This module's own edges, which is all this file states: **none.** This repository is the base every other builds on; it links no sibling.

The family-wide table used to be copied here and into every sibling. Every copy went stale together the day
the two public edges were removed, so it now lives once, in `tge-core/docs/modular-architecture.md`. A
repository states only what it can verify from its own tree.

**There is no cycle today.** tge-renderer names nothing above it — zero references to Animation in its
`CMakeLists.txt`, `public/` or `private/`. Animation → Renderer is a plain downward edge. The reason it works
is Core: `tge-core/public/tge/geometry/vertex.hpp:21,29` owns `SSkinVertex` and `SMorphDelta`, so the Renderer
draws skinned meshes without knowing the Animation module exists. **That is the mechanism every remaining edge
should be dissolved by** — shared vocabulary in Core, typed events for the wiring, no module naming another.

**Why this concerns Core specifically.** This repository is the sole intended exception to the peer rule, and
its vocabulary namespaces are what make the rule achievable. `Tge::Geometry`, `Tge::Material`, `Tge::Light`,
`Tge::Entity` and `Tge::Exposure` exist so producers and consumers can embed the same types by value without
naming each other — and `SSkinVertex` is the working proof, letting the Renderer skin meshes with no
dependency on the Animation module.

**Expect the remaining edge work to propose moving types here.** The nearest candidate is tge-asset-loader's
`model_data.hpp` (`STextureData`, `SMaterialData`, `SMeshData`, `SNodeData`, …), which is the whole of the
Renderer's only interface-level sibling edge and possibly the Scene's too.

Each such move must be weighed against what this repository's own `CLAUDE.md` already states: **a type hosted
here versions with Core, and every module rebuilds against every change to it.** The test for admitting a type
is the one already written down — producers and consumers embed it *by value*, so it can never version
independently of them. A type that fails that test buys peer-ness at the price of coupling everything to Core's
release cadence.

**The second payoff, new on 2026-08-07.** A dev layer is planned for these repos: an app per module that boots
a minimal running environment so the module can be proven at runtime, top-level only, installed never. Such an
app can vendor its *downward* closure today with no cycle. What it cannot do is reach sideways — a Renderer dev
app cannot open a scene with animated models, because Scene and Animation sit above it. **Every edge dissolved
is a dev app that gains that reach**, and once no sibling library edges remain, any dev app may vendor any
combination freely.
