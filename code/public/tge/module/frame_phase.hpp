#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Tge
{
// The per-frame schedule, in execution order. Separate from module registration order, which answers a
// dependency question instead — one list serving both loses the schedule silently whenever they disagree.
enum class EFramePhase : uint8_t
{
	FrameStart,
	Pace,
	Input,
	Simulate,
	Render,
	PostRender,

	Count
};

inline constexpr size_t FramePhaseCount = static_cast<size_t>(EFramePhase::Count);

struct SFramePhaseEntry final
{
	EFramePhase phase;
	std::string_view name;
};

inline constexpr std::array FramePhaseNames
{
	SFramePhaseEntry{ EFramePhase::FrameStart, "FrameStart" },
	SFramePhaseEntry{ EFramePhase::Pace,       "Pace" },
	SFramePhaseEntry{ EFramePhase::Input,      "Input" },
	SFramePhaseEntry{ EFramePhase::Simulate,   "Simulate" },
	SFramePhaseEntry{ EFramePhase::Render,     "Render" },
	SFramePhaseEntry{ EFramePhase::PostRender, "PostRender" }
};

//////////////////////////////////////////////////////////////////////////
inline constexpr std::string_view GetFramePhaseName(EFramePhase phase)
{
	std::string_view name{ "Unknown" };

	for (SFramePhaseEntry const& entry : FramePhaseNames)
	{
		if (entry.phase == phase)
		{
			name = entry.name;
		}
	}

	return name;
}

//////////////////////////////////////////////////////////////////////////
// An unnamed phase would read as "Unknown" in the scheduling error that needs it most.
consteval bool AreFramePhaseNamesComplete()
{
	bool complete{ true };

	for (size_t index{ 0 }; index < FramePhaseCount; ++index)
	{
		complete = complete && (GetFramePhaseName(static_cast<EFramePhase>(index)) != "Unknown");
	}

	return complete;
}

static_assert(AreFramePhaseNamesComplete(), "a frame phase is missing its name");
} // namespace Tge
