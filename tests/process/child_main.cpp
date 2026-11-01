#include <tge/platform.hpp>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <sys/stat.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

// <report> [--print] [--exit=<code>] [--signal=<number>] [--probe=<number|stdout>:<path>] [--wait-for=<path>] -- [arguments...]
namespace
{
constexpr std::chrono::milliseconds PollInterval{ 10 };
constexpr std::chrono::seconds Patience{ 10 };

//////////////////////////////////////////////////////////////////////////
std::string ToUtf8(std::filesystem::path const& path)
{
	std::u8string const text{ path.u8string() };

	return std::string{ reinterpret_cast<char const*>(text.data()), text.size() };
}

//////////////////////////////////////////////////////////////////////////
std::filesystem::path FromUtf8(std::string_view text)
{
	return std::filesystem::path{ std::u8string_view{ reinterpret_cast<char8_t const*>(text.data()), text.size() } };
}

//////////////////////////////////////////////////////////////////////////
bool IsStdinNull()
{
#if defined(TGE_PLATFORM_WINDOWS)
	HANDLE const input{ GetStdHandle(STD_INPUT_HANDLE) };
	DWORD mode{ 0 };

	return GetFileType(input) == FILE_TYPE_CHAR && GetConsoleMode(input, &mode) == FALSE;
#elif defined(TGE_PLATFORM_LINUX)
	struct stat input{};
	struct stat null{};

	return ::fstat(STDIN_FILENO, &input) == 0 && ::stat("/dev/null", &null) == 0 && S_ISCHR(input.st_mode) && input.st_rdev == null.st_rdev;
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}

//////////////////////////////////////////////////////////////////////////
// Open only while the number, or "stdout", still names that file: the same number can be some other handle here.
bool IsProbeOpen(std::string_view probe)
{
	size_t const colon{ std::min(probe.find(':'), probe.size()) };
	bool const isStdout{ probe.substr(0, colon) == "stdout" };
	uint64_t number{ 0 };
	std::filesystem::path held{};
	std::error_code error{};

	std::from_chars(probe.data(), probe.data() + colon, number);

#if defined(TGE_PLATFORM_WINDOWS)
	HANDLE const handle{ isStdout ? GetStdHandle(STD_OUTPUT_HANDLE) : reinterpret_cast<HANDLE>(static_cast<uintptr_t>(number)) };
	std::wstring name{};

	name.resize(32768);

	DWORD const size{ GetFinalPathNameByHandleW(handle, name.data(), static_cast<DWORD>(name.size()), FILE_NAME_NORMALIZED) };

	if (size > 0 && size < name.size())
	{
		name.resize(size);
		held = name;
	}
#elif defined(TGE_PLATFORM_LINUX)
	held = std::filesystem::read_symlink(std::format("/proc/self/fd/{}", isStdout ? STDOUT_FILENO : static_cast<int>(number)), error);
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return !held.empty() && colon < probe.size() && std::filesystem::equivalent(held, FromUtf8(probe.substr(colon + 1)), error);
}

//////////////////////////////////////////////////////////////////////////
void Print()
{
#if defined(TGE_PLATFORM_WINDOWS)
	_setmode(_fileno(stdout), _O_BINARY);
	_setmode(_fileno(stderr), _O_BINARY);
#endif // defined(TGE_PLATFORM_WINDOWS)

	std::fputs("out\n", stdout);
	std::fflush(stdout);
	std::fputs("err\n", stderr);
	std::fflush(stderr);
}

//////////////////////////////////////////////////////////////////////////
void WaitFor(std::filesystem::path const& path)
{
	auto const deadline{ std::chrono::steady_clock::now() + Patience };
	std::error_code error{};

	while (!std::filesystem::exists(path, error) && std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::sleep_for(PollInterval);
	}
}

//////////////////////////////////////////////////////////////////////////
int Run(std::span<std::string const> args)
{
	std::error_code error{};
	std::string report{ std::format("cwd {}\nstdin {}\n", ToUtf8(std::filesystem::current_path(error)), IsStdinNull() ? "null" : "other") };
	bool isPrinting{ false };
	int signal{ 0 };
	int exitCode{ 0 };
	std::filesystem::path waitFor{};
	bool isOption{ true };

#if defined(TGE_PLATFORM_LINUX)
	report += std::format("session {}\n", (::getsid(0) == ::getpid()) ? "own" : "shared");
#endif // defined(TGE_PLATFORM_LINUX)

	for (std::string_view const arg : args.subspan(1))
	{
		if (!isOption)
		{
			report += std::format("arg {}\n", arg);
		}
		else if (arg == "--")
		{
			isOption = false;
		}
		else if (arg == "--print")
		{
			isPrinting = true;
		}
		else if (arg.starts_with("--signal="))
		{
			std::from_chars(arg.data() + 9, arg.data() + arg.size(), signal);
		}
		else if (arg.starts_with("--exit="))
		{
			std::from_chars(arg.data() + 7, arg.data() + arg.size(), exitCode);
		}
		else if (arg.starts_with("--probe="))
		{
			report += std::format("probe {}\n", IsProbeOpen(arg.substr(8)) ? "open" : "closed");
		}
		else if (arg.starts_with("--wait-for="))
		{
			waitFor = FromUtf8(arg.substr(11));
		}
	}

	std::ofstream{ FromUtf8(args.front()), std::ios::binary } << report;

	if (isPrinting)
	{
		Print();
	}

	if (!waitFor.empty())
	{
		WaitFor(waitFor);
	}

	if (signal != 0)
	{
		std::raise(signal);
	}

	return exitCode;
}
} // namespace

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
// Wide, as the narrow arguments would come in the ANSI code page.
int wmain(int argc, wchar_t* argv[])
{
	std::vector<std::string> args{};

	for (wchar_t const* const pArg : std::span{ argv, static_cast<size_t>(argc) }.subspan(1))
	{
		args.emplace_back(ToUtf8(std::filesystem::path{ pArg }));
	}

	return args.empty() ? 1 : Run(args);
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	std::span<char* const> const all{ argv, static_cast<size_t>(argc) };
	std::vector<std::string> const args{ all.begin() + 1, all.end() };

	return args.empty() ? 1 : Run(args);
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
