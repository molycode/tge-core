#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace Tge::Net
{
std::vector<std::vector<std::byte>> DecodeCertificates(std::string_view content);
} // namespace Tge::Net
