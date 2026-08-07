# Dependency notes — tge-core

Split out of the workspace-level `TGE_DEPENDENCY_HANDOFF.md` on 2026-08-06, so each finding sits in the
repository whose session will act on it. Everything here was measured on Thomas's machine, not inferred.
The cross-cutting index lives in `tge-demo/docs/dependency-notes.md`.

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

The rule is now narrowed to `tasks/` alone, so real documentation can live here. **Both of those documents
still sit in `tge-demo/docs/` and moving them is the follow-up** — this only unblocked it.

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
