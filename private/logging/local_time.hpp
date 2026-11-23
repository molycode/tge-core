#pragma once

#include <ctime>

namespace Tge::Logging
{
// The calendar time a moment falls on in the local time zone.
std::tm ToLocalTime(std::time_t time);
} // namespace Tge::Logging
