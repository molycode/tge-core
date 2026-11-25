#pragma once

#include <memory>

namespace Tge
{
class IApp;

// Initializes the app, updates it while it can run, terminates it and returns its exit status. A browser never lets
// main block: there this returns 0 once the app is initialized, the browser calls the frames, and the app's status is
// the program's when the run ends.
int RunFrameLoop(std::unique_ptr<IApp> pApp);
} // namespace Tge
