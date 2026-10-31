#pragma once

#include <cstddef>
#include <expected>
#include <system_error>
#include <vector>

namespace Tge::Net
{
std::expected<std::vector<std::vector<std::byte>>, std::error_code> ReadSystemRootCertificates();
} // namespace Tge::Net
