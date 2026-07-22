#pragma once

#include <tge/non_copyable.hpp>

namespace Tge
{
struct SRunContext;

// Update clamps its delta to this before ticking anything: a stall (shader compile, asset load, an IBL bake)
// hands it the stall's whole duration, and an unclamped delta would drive every clip that far in one frame.
// Below the resulting ~10 FPS the simulation slows down rather than teleporting through the hitch.
inline constexpr float MaxFrameDelta{ 0.1f };

class IRuntime : public SNoCopyNoMove
{
public:

	virtual bool Initialize(SRunContext const& context) = 0;
	virtual void Terminate() = 0;

	// deltaTime is clamped to MaxFrameDelta, once, so every module advances by the same bounded timestep.
	virtual void Update(float deltaTime) = 0;

	virtual void Quit() = 0;
	virtual bool CanRun() const = 0;

protected:

	~IRuntime() = default;
};

extern IRuntime* const gRuntime;
} // namespace Tge
