#pragma once

#include <tge/module/frame_phase.hpp>
#include <span>

namespace Tge
{
class IModule;

struct SScheduleEntry final
{
	EFramePhase phase{ EFramePhase::FrameStart };
	IModule*    pModule{ nullptr };
};

// A static table, so a context costs no parser and no boot-time failure mode of its own.
struct SRunContext final
{
	// Unordered — the runtime derives initialization order from the declared dependencies, so a context
	// author cannot restate an ordering constraint wrongly here.
	std::span<IModule* const>       modules;
	std::span<SScheduleEntry const> schedule;
	std::span<IModule* const>       noTick;
};
} // namespace Tge
