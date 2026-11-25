#pragma once

#include <system_error>

namespace Tge::IO
{
// What was written to the open file outlives a crash once this returns, where the system has such a call.
std::error_code FlushFile(int file);
} // namespace Tge::IO
