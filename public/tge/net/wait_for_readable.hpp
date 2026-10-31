#pragma once

#include <tge/threading/waitable.hpp>
#include <chrono>
#include <expected>
#include <system_error>

namespace Tge::Net
{
std::expected<bool, std::error_code> WaitForReadable(Threading::SWaitable socket, std::chrono::milliseconds timeout);
} // namespace Tge::Net
