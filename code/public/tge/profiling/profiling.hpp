#pragma once

#include <tge/config.hpp>

// TGE-owned CPU profiling vocabulary. Engine code uses ONLY the TGE_PROFILE_* markers below — the profiling
// backend is selected HERE and nowhere else, so the engine is never married to a vendor. This is the durable
// "seam": instrument once with these markers, swap/extend backends without touching call sites.
//
// Backend model:
//  - External profilers (Tracy now; PIX/Razor per console later) are SOURCE-SITE macro profilers: they self-
//    timestamp at the call site and need static source-location data, so they are partnered at COMPILE time
//    (selected per platform via the CMake toolchain), not routed through a runtime vtable.
//  - The in-engine aggregator (coarse, always-on "is this system faster/slower" view) and its IProfilerBackend
//    runtime interface land later, alongside that aggregator — these markers already give it its hook point.
//
// TGE_PROFILE_TRACY is defined by the TgeProfiling target when a Tracy client target exists in the build.

#if defined(TGE_PROFILE_TRACY)
#include <tge/non_copyable.hpp>
#include <tracy/Tracy.hpp>

#include <chrono>

// Mark the end of a frame (call once per presented frame).
#define TGE_PROFILE_FRAME()       FrameMark
// Scope timer for the enclosing block, auto-named from the function.
#define TGE_PROFILE_SCOPE()       ZoneScoped
// Scope timer for the enclosing block with an explicit literal name.
#define TGE_PROFILE_SCOPE_N(name) ZoneScopedN(name)
// Plot a named numeric value on the timeline.
#define TGE_PROFILE_PLOT(name, value) TracyPlot(name, value)

namespace Tge::Profiling
{
// A plot rather than a zone: dynamically named zones share the macro site's one source location and collapse
// into a single csvexport row, so spans timed from the same line become indistinguishable there.
// Ctor/dtor do the work because measuring entry to exit is the whole contract, as Tracy's own ScopedZone does.
class CScopedPlotMs final : private SNoCopyNoMove
{
public:

	explicit CScopedPlotMs(char const* pName)
		: m_pName{ pName }
		, m_start{ std::chrono::steady_clock::now() }
	{
	}

	~CScopedPlotMs()
	{
		std::chrono::duration<double, std::milli> const elapsed{ std::chrono::steady_clock::now() - m_start };
		TracyPlot(m_pName, elapsed.count());
	}

private:

	char const* m_pName{ nullptr };
	std::chrono::steady_clock::time_point m_start;
};
} // namespace Tge::Profiling

// Tracy transmits the name POINTER and resolves the string out of this process later, so the name must be
// null-terminated and outlive the trace -- a literal, never a temporary or a substring.
#define TGE_PROFILE_PLOT_SCOPE_MS(name) Tge::Profiling::CScopedPlotMs TracyConcat(_tgeScopedPlot, TracyLine){ name }
#else
#define TGE_PROFILE_FRAME()       ((void)0)
#define TGE_PROFILE_SCOPE()       ((void)0)
#define TGE_PROFILE_SCOPE_N(name) ((void)0)
#define TGE_PROFILE_PLOT(name, value) ((void)0)
#define TGE_PROFILE_PLOT_SCOPE_MS(name) ((void)0)
#endif // TGE_PROFILE_TRACY
