#pragma once

#include <string_view>

namespace Tge::Threading
{
// Names the calling thread where the operating system shows thread names (debuggers, task managers).
void SetOsThreadName(std::string_view name);
} // namespace Tge::Threading
