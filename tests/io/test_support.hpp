#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>

namespace Tge::IO::Tests
{
inline constexpr std::chrono::seconds Patience{ 10 };

std::filesystem::path MakeTemporaryFolder(std::string_view prefix);
void WriteText(std::filesystem::path const& path, std::string_view text);
std::string ReadText(std::filesystem::path const& path);
size_t CountEntries(std::filesystem::path const& folder);
} // namespace Tge::IO::Tests
