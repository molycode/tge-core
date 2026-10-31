#pragma once

#include <system_error>

namespace Tge::IO
{
bool IsBusy(std::error_code const& error);
} // namespace Tge::IO
