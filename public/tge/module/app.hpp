#pragma once

#include <tge/non_copyable.hpp>

namespace Tge
{
// What RunFrameLoop runs. It holds whatever the frames need: in a browser main's stack is gone before the first one.
class IApp : public SNoCopyNoMove
{
public:

	virtual ~IApp() = default;

	// False ends the run without a frame; Terminate is called all the same.
	virtual bool Initialize() = 0;

	// Seconds since the previous frame, unclamped: a browser runs no frames while its page is hidden.
	virtual void Update(float deltaTime) = 0;
	virtual bool CanRun() const = 0;

	// Returns what the process exits with.
	virtual int Terminate() = 0;
};
} // namespace Tge
