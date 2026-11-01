#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/platform.hpp>
#include <tge/process/child_process.hpp>
#include <tge/process/output_file.hpp>
#include <tge/process/process_spec.hpp>
#include <expected>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <cstdint>
#elif defined(TGE_PLATFORM_LINUX)
#include <csignal>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

using namespace Tge::Process;
using namespace Tge::Process::Tests;

namespace
{
#if defined(TGE_PLATFORM_WINDOWS)
constexpr char PathListSeparator{ ';' };
#elif defined(TGE_PLATFORM_LINUX)
constexpr char PathListSeparator{ ':' };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

//////////////////////////////////////////////////////////////////////////
class CChildProcessTest : public testing::Test
{
protected:

	// testing::Test
	void SetUp() override
	{
		m_folder = MakeTemporaryFolder("tge-process");
		ASSERT_FALSE(m_folder.empty());
		m_report = m_folder / "report.txt";
	}

	void TearDown() override
	{
		std::error_code error{};

		m_child.Terminate();
		m_output.Close();
		std::filesystem::remove_all(m_folder, error);
	}
	// ~testing::Test

	SProcessSpec MakeSpec(std::vector<std::string> const& options, std::vector<std::string> const& arguments) const
	{
		SProcessSpec spec{};

		spec.program = GetChildProgram();
		spec.arguments.emplace_back(ToUtf8(m_report));
		spec.arguments.insert(spec.arguments.end(), options.begin(), options.end());
		spec.arguments.emplace_back("--");
		spec.arguments.insert(spec.arguments.end(), arguments.begin(), arguments.end());

		return spec;
	}

	// The child's report, once it has ended with code 0.
	std::string RunToEnd(SProcessSpec const& spec)
	{
		std::string report{};
		std::expected<void, std::error_code> const started{ m_child.Start(spec) };

		EXPECT_TRUE(started.has_value()) << started.error().message();

		if (started.has_value())
		{
			std::optional<std::expected<SExitStatus, std::error_code>> const exit{ WaitForExit(m_child) };

			EXPECT_TRUE(exit.has_value() && exit->has_value() && (*exit)->code == 0);
			report = ReadText(m_report);
		}

		return report;
	}

	std::string WaitForSignal()
	{
		std::optional<std::expected<SExitStatus, std::error_code>> const exit{ WaitForExit(m_child) };

		return (exit.has_value() && exit->has_value()) ? (*exit)->signal : std::string{ "(no end)" };
	}

	std::filesystem::path m_folder{};
	std::filesystem::path m_report{};
	CChildProcess m_child{};
	COutputFile m_output{};
};
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, ArgumentsArriveAsGiven)
{
	std::vector<std::string> const arguments{ "plain", "with space", "with \"quote\"", "back\\slash", "trailing\\", "", "\xC3\xA9t\xC3\xA9",
		"+connect" };

	EXPECT_EQ(FindReportArguments(RunToEnd(MakeSpec({}, arguments))), arguments);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, StartsInTheWorkingDirectory)
{
	SProcessSpec spec{ MakeSpec({}, {}) };
	std::error_code error{};

	spec.workingDir = m_folder;

	std::string const report{ RunToEnd(spec) };
	std::string const cwd{ FindReportValue(report, "cwd") };

	EXPECT_TRUE(std::filesystem::equivalent(std::filesystem::path{ std::u8string_view{ reinterpret_cast<char8_t const*>(cwd.data()), cwd.size() } },
		m_folder, error)) << cwd;
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, RelativeProgramIsFoundInTheWorkingDirectory)
{
	SProcessSpec spec{ MakeSpec({}, {}) };

	spec.workingDir = GetChildProgram().parent_path();
	spec.program = std::filesystem::path{ "." } / GetChildProgram().filename();

	EXPECT_FALSE(RunToEnd(spec).empty());
}

//////////////////////////////////////////////////////////////////////////
// In front of the rest, which a child built with a sanitizer needs to find its runtime.
TEST_F(CChildProcessTest, BareNameIsFoundThroughPath)
{
	std::optional<std::string> const searchPath{ GetVariable("PATH") };
	SProcessSpec spec{ MakeSpec({}, {}) };

	spec.program = GetChildProgram().stem();
	SetVariable("PATH", std::format("{}{}{}", ToUtf8(GetChildProgram().parent_path()), PathListSeparator, searchPath.value_or(std::string{})));

	std::string const report{ RunToEnd(spec) };

	SetVariable("PATH", searchPath);

	EXPECT_FALSE(report.empty());
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, StdinIsTheNullDevice)
{
	EXPECT_EQ(FindReportValue(RunToEnd(MakeSpec({}, {})), "stdin"), "null");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, OutputAndErrorsLandInTheOutputFile)
{
	std::filesystem::path const outputPath{ m_folder / "output.log" };
	SProcessSpec spec{ MakeSpec({ "--print" }, {}) };

	ASSERT_TRUE(m_output.Open(outputPath).has_value());
	spec.pOutput = &m_output;
	RunToEnd(spec);
	m_output.Close();

	EXPECT_EQ(ReadText(outputPath), "out\nerr\n");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, WithoutAnOutputFileTheOutputIsDiscarded)
{
	EXPECT_FALSE(RunToEnd(MakeSpec({ "--print" }, {})).empty());
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, OutputFileOpensAgainWhileInUse)
{
	COutputFile second{};

	ASSERT_TRUE(m_output.Open(m_folder / "output.log").has_value());

	EXPECT_TRUE(second.Open(m_folder / "output.log").has_value());

	second.Close();
}

//////////////////////////////////////////////////////////////////////////
// Also what shows that the probe below sees an open file at all.
TEST_F(CChildProcessTest, OutputFileIsTheChildsStdout)
{
	std::filesystem::path const outputPath{ m_folder / "output.log" };
	SProcessSpec spec{ MakeSpec({ std::format("--probe=stdout:{}", ToUtf8(outputPath)) }, {}) };

	ASSERT_TRUE(m_output.Open(outputPath).has_value());
	spec.pOutput = &m_output;

	EXPECT_EQ(FindReportValue(RunToEnd(spec), "probe"), "open");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, FilesTheParentHoldsDoNotReachTheChild)
{
	std::filesystem::path const probePath{ m_folder / "probe.txt" };
	std::string probe{};

	WriteText(probePath, "probe");

#if defined(TGE_PLATFORM_WINDOWS)
	SECURITY_ATTRIBUTES inheritable{ static_cast<DWORD>(sizeof(SECURITY_ATTRIBUTES)), nullptr, TRUE };
	HANDLE const held{ CreateFileW(probePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, &inheritable, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
		nullptr) };

	ASSERT_NE(held, INVALID_HANDLE_VALUE);
	probe = std::format("--probe={}:{}", reinterpret_cast<uintptr_t>(held), ToUtf8(probePath));
#elif defined(TGE_PLATFORM_LINUX)
	int const held{ ::open(probePath.c_str(), O_RDONLY) };

	ASSERT_GE(held, 0);
	probe = std::format("--probe={}:{}", held, ToUtf8(probePath));
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	std::string const report{ RunToEnd(MakeSpec({ probe }, {})) };

#if defined(TGE_PLATFORM_WINDOWS)
	CloseHandle(held);
#elif defined(TGE_PLATFORM_LINUX)
	::close(held);
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	EXPECT_EQ(FindReportValue(report, "probe"), "closed");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, ExitCodeIsReported)
{
	ASSERT_TRUE(m_child.Start(MakeSpec({ "--exit=3" }, {})).has_value());

	std::optional<std::expected<SExitStatus, std::error_code>> const exit{ WaitForExit(m_child) };

	ASSERT_TRUE(exit.has_value() && exit->has_value());
	EXPECT_EQ((*exit)->code, 3u);
	EXPECT_TRUE((*exit)->signal.empty());
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, RunningChildHasNoExitYet)
{
	std::filesystem::path const release{ m_folder / "release" };

	ASSERT_TRUE(m_child.Start(MakeSpec({ std::format("--wait-for={}", ToUtf8(release)) }, {})).has_value());

	EXPECT_FALSE(m_child.TryGetExit().has_value());

	WriteText(release, "go");

	std::optional<std::expected<SExitStatus, std::error_code>> const exit{ WaitForExit(m_child) };

	EXPECT_TRUE(exit.has_value() && exit->has_value());
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, MissingProgramIsNotFound)
{
	SProcessSpec spec{ MakeSpec({}, {}) };

	spec.program = m_folder / std::filesystem::path{ "missing" }.replace_extension(GetChildProgram().extension());

	std::expected<void, std::error_code> const started{ m_child.Start(spec) };

	ASSERT_FALSE(started.has_value());
	EXPECT_EQ(started.error(), std::errc::no_such_file_or_directory) << started.error().message();
}

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
// cmd.exe would run it and parse its arguments again.
TEST_F(CChildProcessTest, BatchFileIsRefusedByPath)
{
	SProcessSpec spec{ MakeSpec({}, {}) };

	WriteText(m_folder / "script.bat", "@echo off\r\n");
	spec.program = m_folder / "script.bat";

	std::expected<void, std::error_code> const started{ m_child.Start(spec) };

	ASSERT_FALSE(started.has_value());
	EXPECT_EQ(started.error(), std::errc::executable_format_error) << started.error().message();
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, BatchFileIsRefusedByName)
{
	std::optional<std::string> const searchPath{ GetVariable("PATH") };
	SProcessSpec spec{ MakeSpec({}, {}) };

	WriteText(m_folder / "script.bat", "@echo off\r\n");
	spec.program = "script.bat";
	SetVariable("PATH", ToUtf8(m_folder));

	std::expected<void, std::error_code> const started{ m_child.Start(spec) };

	SetVariable("PATH", searchPath);

	ASSERT_FALSE(started.has_value());
	EXPECT_EQ(started.error(), std::errc::executable_format_error) << started.error().message();
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, ChildGetsASessionOfItsOwn)
{
	EXPECT_EQ(FindReportValue(RunToEnd(MakeSpec({}, {})), "session"), "own");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, SignalIsNamed)
{
	ASSERT_TRUE(m_child.Start(MakeSpec({ std::format("--signal={}", SIGTERM) }, {})).has_value());

	EXPECT_EQ(WaitForSignal(), "SIGTERM");
}

//////////////////////////////////////////////////////////////////////////
// Ignored signals are inherited: only a reset to the default lets the child die of the one it raises.
TEST_F(CChildProcessTest, IgnoredSignalStartsAtItsDefault)
{
	struct sigaction ignore{};
	struct sigaction saved{};

	ignore.sa_handler = SIG_IGN;
	::sigaction(SIGPIPE, &ignore, &saved);

	std::expected<void, std::error_code> const started{ m_child.Start(MakeSpec({ std::format("--signal={}", SIGPIPE) }, {})) };

	::sigaction(SIGPIPE, &saved, nullptr);
	ASSERT_TRUE(started.has_value());
	EXPECT_EQ(WaitForSignal(), "SIGPIPE");
}

//////////////////////////////////////////////////////////////////////////
// Left blocked, the signal would wait until the child exited cleanly.
TEST_F(CChildProcessTest, BlockedSignalStartsUnblocked)
{
	sigset_t blocked{};
	sigset_t saved{};

	sigemptyset(&blocked);
	sigaddset(&blocked, SIGUSR2);
	::pthread_sigmask(SIG_BLOCK, &blocked, &saved);

	std::expected<void, std::error_code> const started{ m_child.Start(MakeSpec({ std::format("--signal={}", SIGUSR2) }, {})) };

	::pthread_sigmask(SIG_SETMASK, &saved, nullptr);
	ASSERT_TRUE(started.has_value());
	EXPECT_EQ(WaitForSignal(), "SIGUSR2");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CChildProcessTest, ChildReapedElsewhereIsLost)
{
	ASSERT_TRUE(m_child.Start(MakeSpec({}, {})).has_value());
	ASSERT_EQ(::waitpid(static_cast<pid_t>(m_child.GetId()), nullptr, 0), static_cast<pid_t>(m_child.GetId()));

	std::optional<std::expected<SExitStatus, std::error_code>> const exit{ m_child.TryGetExit() };

	ASSERT_TRUE(exit.has_value());
	ASSERT_FALSE(exit->has_value());
	EXPECT_EQ(exit->error(), std::errc::no_child_process);
}

//////////////////////////////////////////////////////////////////////////
// With the caller's stdin closed the output file gets descriptor 0, which the child's own stdin must not replace.
TEST_F(CChildProcessTest, OutputIsKeptWithStdinClosed)
{
	std::filesystem::path const outputPath{ m_folder / "output.log" };
	SProcessSpec spec{ MakeSpec({ "--print" }, {}) };
	int const savedStdin{ ::dup(STDIN_FILENO) };

	ASSERT_GE(savedStdin, 0);
	::close(STDIN_FILENO);

	std::expected<void, std::error_code> const opened{ m_output.Open(outputPath) };

	spec.pOutput = &m_output;

	if (opened.has_value())
	{
		RunToEnd(spec);
	}

	m_output.Close();
	::dup2(savedStdin, STDIN_FILENO);
	::close(savedStdin);

	ASSERT_TRUE(opened.has_value());
	EXPECT_EQ(ReadText(outputPath), "out\nerr\n");
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
