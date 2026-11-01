#include "test_support.hpp"
#include <tge/platform.hpp>
#include <format>
#include <fstream>
#include <random>
#include <sstream>
#include <thread>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <cstdlib>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Process::Tests
{
namespace
{
constexpr std::chrono::milliseconds PollInterval{ 10 };
} // namespace

//////////////////////////////////////////////////////////////////////////
std::filesystem::path GetChildProgram()
{
	return std::filesystem::path{ TGE_PROCESS_CHILD };
}

//////////////////////////////////////////////////////////////////////////
// Empty when no folder could be made.
std::filesystem::path MakeTemporaryFolder(std::string_view prefix)
{
	std::error_code error{};
	std::filesystem::path const base{ std::filesystem::temp_directory_path(error) };
	std::random_device random{};
	std::filesystem::path made{};

	for (int attempt{ 0 }; attempt < 100 && made.empty() && error == std::error_code{}; ++attempt)
	{
		std::filesystem::path const candidate{ base / std::format("{}-{:08x}", prefix, random()) };

		if (std::filesystem::create_directory(candidate, error))
		{
			made = candidate;
		}
	}

	return made;
}

//////////////////////////////////////////////////////////////////////////
void WriteText(std::filesystem::path const& path, std::string_view text)
{
	std::ofstream{ path, std::ios::binary } << text;
}

//////////////////////////////////////////////////////////////////////////
std::string ReadText(std::filesystem::path const& path)
{
	std::ifstream stream{ path, std::ios::binary };
	std::ostringstream text{};

	text << stream.rdbuf();

	return text.str();
}

//////////////////////////////////////////////////////////////////////////
std::string ToUtf8(std::filesystem::path const& path)
{
	std::u8string const text{ path.u8string() };

	return std::string{ reinterpret_cast<char const*>(text.data()), text.size() };
}

//////////////////////////////////////////////////////////////////////////
std::string FindReportValue(std::string_view report, std::string_view key)
{
	std::string value{};
	std::istringstream lines{ std::string{ report } };
	std::string line{};
	std::string const prefix{ std::format("{} ", key) };

	while (std::getline(lines, line))
	{
		if (value.empty() && line.starts_with(prefix))
		{
			value = line.substr(prefix.size());
		}
	}

	return value;
}

//////////////////////////////////////////////////////////////////////////
std::vector<std::string> FindReportArguments(std::string_view report)
{
	std::vector<std::string> arguments{};
	std::istringstream lines{ std::string{ report } };
	std::string line{};

	while (std::getline(lines, line))
	{
		if (line.starts_with("arg "))
		{
			arguments.emplace_back(line.substr(4));
		}
	}

	return arguments;
}

//////////////////////////////////////////////////////////////////////////
// Empty when the child did not end in time.
std::optional<std::expected<SExitStatus, std::error_code>> WaitForExit(CChildProcess& child)
{
	auto const deadline{ std::chrono::steady_clock::now() + Patience };
	std::optional<std::expected<SExitStatus, std::error_code>> exit{ child.TryGetExit() };

	while (!exit.has_value() && std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::sleep_for(PollInterval);
		exit = child.TryGetExit();
	}

	return exit;
}

//////////////////////////////////////////////////////////////////////////
// Windows: the system's own copy, which is what the search for a program reads, not the C runtime's.
std::optional<std::string> GetVariable(char const* pName)
{
	std::optional<std::string> value{};

#if defined(TGE_PLATFORM_WINDOWS)
	std::wstring const name{ std::filesystem::path{ pName }.wstring() };
	DWORD const size{ GetEnvironmentVariableW(name.c_str(), nullptr, 0) };

	if (size > 0)
	{
		std::wstring wide{};

		wide.resize(size);
		wide.resize(GetEnvironmentVariableW(name.c_str(), wide.data(), size));
		value = ToUtf8(std::filesystem::path{ wide });
	}
#elif defined(TGE_PLATFORM_LINUX)
	char const* const pValue{ std::getenv(pName) };

	if (pValue != nullptr)
	{
		value = std::string{ pValue };
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return value;
}

//////////////////////////////////////////////////////////////////////////
void SetVariable(char const* pName, std::optional<std::string> const& value)
{
#if defined(TGE_PLATFORM_WINDOWS)
	std::wstring const name{ std::filesystem::path{ pName }.wstring() };
	std::wstring const wide{ value.has_value() ? std::filesystem::path{ std::u8string_view{ reinterpret_cast<char8_t const*>(value->data()), value->size() } }.wstring()
		: std::wstring{} };

	SetEnvironmentVariableW(name.c_str(), value.has_value() ? wide.c_str() : nullptr);
#elif defined(TGE_PLATFORM_LINUX)
	if (value.has_value())
	{
		setenv(pName, value->c_str(), 1);
	}
	else
	{
		unsetenv(pName);
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}
} // namespace Tge::Process::Tests
