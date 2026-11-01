#pragma once

#include <tge/process/child_process.hpp>
#include <chrono>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace Tge::Process::Tests
{
inline constexpr std::chrono::seconds Patience{ 10 };

std::filesystem::path GetChildProgram();
std::filesystem::path MakeTemporaryFolder(std::string_view prefix);
void WriteText(std::filesystem::path const& path, std::string_view text);
std::string ReadText(std::filesystem::path const& path);
std::string ToUtf8(std::filesystem::path const& path);
std::string FindReportValue(std::string_view report, std::string_view key);
std::vector<std::string> FindReportArguments(std::string_view report);
std::optional<std::expected<SExitStatus, std::error_code>> WaitForExit(CChildProcess& child);
std::optional<std::string> GetVariable(char const* pName);
void SetVariable(char const* pName, std::optional<std::string> const& value);
} // namespace Tge::Process::Tests
