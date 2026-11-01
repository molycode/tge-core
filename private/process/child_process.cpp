#include <tge/process/child_process.hpp>
#include "native_child_process.hpp"
#include "native_output_file.hpp"
#include <tge/assert.hpp>
#include <tge/process/output_file.hpp>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#if defined(TGE_PLATFORM_WINDOWS)
#include "command_line.hpp"
#include "startable_extension.hpp"
#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>
#elif defined(TGE_PLATFORM_LINUX)
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Process
{
namespace
{
#if defined(TGE_PLATFORM_WINDOWS)
constexpr std::wstring_view DefaultCompletions{ L".COM;.EXE" };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
std::wstring ToWide(std::string_view text)
{
	std::wstring wide{};

	if (!text.empty())
	{
		int const size{ MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0) };

		wide.resize(static_cast<size_t>(size));
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
	}

	return wide;
}

//////////////////////////////////////////////////////////////////////////
std::string ToUtf8(std::filesystem::path const& path)
{
	std::u8string const text{ path.u8string() };

	return std::string{ reinterpret_cast<char const*>(text.data()), text.size() };
}

//////////////////////////////////////////////////////////////////////////
std::wstring ReadVariable(wchar_t const* pName)
{
	std::wstring value{};
	DWORD const size{ GetEnvironmentVariableW(pName, nullptr, 0) };

	if (size > 0)
	{
		value.resize(size);
		value.resize(GetEnvironmentVariableW(pName, value.data(), size));
	}

	return value;
}

//////////////////////////////////////////////////////////////////////////
std::vector<std::wstring_view> SplitList(std::wstring_view list)
{
	std::vector<std::wstring_view> entries{};
	size_t start{ 0 };

	while (start <= list.size())
	{
		size_t const end{ std::min(list.find(L';', start), list.size()) };

		entries.emplace_back(list.substr(start, end - start));
		start = end + 1;
	}

	return entries;
}

//////////////////////////////////////////////////////////////////////////
// PATHEXT's order, kept to the extensions Windows starts without cmd.exe.
std::vector<std::wstring> GetCompletions()
{
	std::wstring const pathExt{ ReadVariable(L"PATHEXT") };
	std::vector<std::wstring> completions{};

	for (std::wstring_view const extension : SplitList(pathExt.empty() ? DefaultCompletions : std::wstring_view{ pathExt }))
	{
		if (IsStartableExtension(extension))
		{
			completions.emplace_back(extension);
		}
	}

	return completions;
}

//////////////////////////////////////////////////////////////////////////
// As cmd.exe finds a bare name, but never in the current folder. Found only with another extension, a batch file
// say, it is refused rather than reported missing.
std::expected<std::filesystem::path, std::error_code> FindInPath(std::filesystem::path const& name)
{
	std::wstring const searchPath{ ReadVariable(L"PATH") };
	bool const isComplete{ IsStartableExtension(name.extension().native()) };
	std::vector<std::wstring> const completions{ isComplete ? std::vector<std::wstring>{ std::wstring{} } : GetCompletions() };
	std::expected<std::filesystem::path, std::error_code> result{ std::unexpected{ std::make_error_code(std::errc::no_such_file_or_directory) } };
	bool isFound{ false };
	bool isUnstartable{ false };
	std::error_code error{};

	for (std::wstring_view entry : SplitList(searchPath))
	{
		bool const isQuoted{ entry.size() >= 2 && entry.front() == L'"' && entry.back() == L'"' };
		std::filesystem::path const folder{ isQuoted ? entry.substr(1, entry.size() - 2) : entry };

		if (!isFound && folder.is_absolute())
		{
			for (std::wstring const& completion : completions)
			{
				std::filesystem::path const candidate{ folder / (name.native() + completion) };

				if (!isFound && std::filesystem::is_regular_file(candidate, error))
				{
					result = candidate;
					isFound = true;
				}
			}

			isUnstartable = isUnstartable || (!isComplete && std::filesystem::is_regular_file(folder / name, error));
		}
	}

	if (!isFound && isUnstartable)
	{
		result = std::unexpected{ std::make_error_code(std::errc::executable_format_error) };
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
// Missing before unstartable, so a mistyped name says so whatever its extension.
std::expected<std::filesystem::path, std::error_code> CheckProgram(std::filesystem::path const& program)
{
	std::error_code error{};
	std::filesystem::file_status const status{ std::filesystem::status(program, error) };
	std::expected<std::filesystem::path, std::error_code> result{ program };

	if (!std::filesystem::exists(status))
	{
		result = std::unexpected{ std::make_error_code(std::errc::no_such_file_or_directory) };
	}
	else if (std::filesystem::is_directory(status))
	{
		result = std::unexpected{ std::make_error_code(std::errc::is_a_directory) };
	}
	else if (!IsStartableExtension(program.extension().native()))
	{
		result = std::unexpected{ std::make_error_code(std::errc::executable_format_error) };
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
std::expected<std::filesystem::path, std::error_code> ResolveProgram(std::filesystem::path const& program, std::filesystem::path const& workingDir)
{
	bool const isBare{ !program.has_root_name() && !program.has_root_directory() && !program.has_parent_path() };
	std::error_code error{};
	std::filesystem::path const base{ workingDir.empty() ? std::filesystem::current_path(error) : workingDir };

	return isBare ? FindInPath(program) : CheckProgram(program.is_absolute() ? program : (base / program).lexically_normal());
}

//////////////////////////////////////////////////////////////////////////
// Only the child's own standard handles are inherited, so nothing else the caller holds open, a socket say, reaches
// it. Those must be inheritable, and the list refuses a handle named twice.
std::error_code CreateChild(SNativeChildProcess& child, std::filesystem::path const& program, std::span<std::string const> arguments,
	std::filesystem::path const& workingDir, HANDLE output)
{
	SECURITY_ATTRIBUTES inheritable{ static_cast<DWORD>(sizeof(SECURITY_ATTRIBUTES)), nullptr, TRUE };
	HANDLE const nul{ CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &inheritable, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr) };
	HANDLE childOutput{ INVALID_HANDLE_VALUE };
	std::error_code error{ (nul != INVALID_HANDLE_VALUE) ? std::error_code{} : GetLastSystemError() };
	std::vector<std::byte> listBuffer{};
	LPPROC_THREAD_ATTRIBUTE_LIST pList{ nullptr };
	std::vector<HANDLE> inherited{};

	if (error == std::error_code{} && output != INVALID_HANDLE_VALUE
		&& DuplicateHandle(GetCurrentProcess(), output, GetCurrentProcess(), &childOutput, 0, TRUE, DUPLICATE_SAME_ACCESS) == FALSE)
	{
		error = GetLastSystemError();
	}

	if (error == std::error_code{})
	{
		SIZE_T listSize{ 0 };

		InitializeProcThreadAttributeList(nullptr, 1, 0, &listSize);
		listBuffer.resize(listSize);

		if (InitializeProcThreadAttributeList(reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(listBuffer.data()), 1, 0, &listSize) != FALSE)
		{
			pList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(listBuffer.data());
		}
		else
		{
			error = GetLastSystemError();
		}
	}

	if (error == std::error_code{})
	{
		inherited.emplace_back(nul);

		if (childOutput != INVALID_HANDLE_VALUE)
		{
			inherited.emplace_back(childOutput);
		}

		if (UpdateProcThreadAttribute(pList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited.data(), inherited.size() * sizeof(HANDLE), nullptr, nullptr) == FALSE)
		{
			error = GetLastSystemError();
		}
	}

	if (error == std::error_code{})
	{
		std::wstring commandLine{ ToWide(BuildCommandLine(ToUtf8(program), arguments)) };
		STARTUPINFOEXW startup{};
		PROCESS_INFORMATION info{};

		startup.StartupInfo.cb = static_cast<DWORD>(sizeof(startup));
		startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
		startup.StartupInfo.hStdInput = nul;
		startup.StartupInfo.hStdOutput = (childOutput != INVALID_HANDLE_VALUE) ? childOutput : nul;
		startup.StartupInfo.hStdError = startup.StartupInfo.hStdOutput;
		startup.lpAttributeList = pList;

		// A hidden console of its own keeps the caller's Ctrl+C away from a console program; a new process group
		// would instead turn Ctrl+C off in the child and everything it starts.
		if (CreateProcessW(program.c_str(), commandLine.data(), nullptr, nullptr, TRUE, EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr,
			workingDir.empty() ? nullptr : workingDir.c_str(), &startup.StartupInfo, &info) != FALSE)
		{
			CloseHandle(info.hThread);
			child.process = info.hProcess;
			child.id = info.dwProcessId;
		}
		else
		{
			error = GetLastSystemError();
		}
	}

	if (pList != nullptr)
	{
		DeleteProcThreadAttributeList(pList);
	}

	if (childOutput != INVALID_HANDLE_VALUE)
	{
		CloseHandle(childOutput);
	}

	if (nul != INVALID_HANDLE_VALUE)
	{
		CloseHandle(nul);
	}

	return error;
}
#elif defined(TGE_PLATFORM_LINUX)
constexpr char const* DevNull{ "/dev/null" };
constexpr int FirstUnusedDescriptor{ STDERR_FILENO + 1 };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ errno, std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
std::string GetSignalName(int signal)
{
	char const* const pName{ ::sigabbrev_np(signal) };

	return (pName != nullptr) ? std::format("SIG{}", pName) : std::format("signal {}", signal);
}

//////////////////////////////////////////////////////////////////////////
std::error_code Spawn(SNativeChildProcess& child, SProcessSpec const& spec, int output)
{
	std::string const program{ spec.program.string() };
	std::vector<char*> argv{};
	posix_spawn_file_actions_t actions{};
	posix_spawnattr_t attributes{};
	sigset_t noSignals{};
	sigset_t allSignals{};

	argv.emplace_back(const_cast<char*>(program.c_str()));

	for (std::string const& argument : spec.arguments)
	{
		argv.emplace_back(const_cast<char*>(argument.c_str()));
	}

	argv.emplace_back(nullptr);
	sigemptyset(&noSignals);
	sigfillset(&allSignals);

	int error{ posix_spawn_file_actions_init(&actions) };
	bool const hasActions{ error == 0 };

	if (hasActions)
	{
		error = posix_spawnattr_init(&attributes);
	}

	bool const hasAttributes{ hasActions && error == 0 };

	if (hasAttributes)
	{
		auto const apply{ [&error](int code)
		{
			error = (error == 0) ? code : error;
		} };

		// Output first: with the caller's stdin closed the output can be descriptor 0, which opening stdin replaces.
		if (output >= 0)
		{
			apply(posix_spawn_file_actions_adddup2(&actions, output, STDOUT_FILENO));
		}
		else
		{
			apply(posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, DevNull, O_WRONLY, 0));
		}

		apply(posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO));
		apply(posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, DevNull, O_RDONLY, 0));

		if (!spec.workingDir.empty())
		{
			apply(posix_spawn_file_actions_addchdir_np(&actions, spec.workingDir.c_str()));
		}

		apply(posix_spawn_file_actions_addclosefrom_np(&actions, FirstUnusedDescriptor));
		apply(posix_spawnattr_setflags(&attributes, static_cast<short>(POSIX_SPAWN_SETSID | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF)));
		apply(posix_spawnattr_setsigmask(&attributes, &noSignals));
		// Ignored signals are inherited, and callers do ignore SIGPIPE; an ignored SIGCHLD would break a child's waits.
		apply(posix_spawnattr_setsigdefault(&attributes, &allSignals));

		if (error == 0)
		{
			error = program.contains('/')
				? posix_spawn(&child.pid, program.c_str(), &actions, &attributes, argv.data(), environ)
				: posix_spawnp(&child.pid, program.c_str(), &actions, &attributes, argv.data(), environ);
		}
	}

	if (hasAttributes)
	{
		posix_spawnattr_destroy(&attributes);
	}

	if (hasActions)
	{
		posix_spawn_file_actions_destroy(&actions);
	}

	return std::error_code{ error, std::system_category() };
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
CChildProcess::CChildProcess() = default;

//////////////////////////////////////////////////////////////////////////
CChildProcess::~CChildProcess() = default;

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CChildProcess::Start(SProcessSpec const& spec)
{
	TGE_ASSERT(m_pProcess == nullptr, "Child process started twice");
	TGE_ASSERT(spec.pOutput == nullptr || spec.pOutput->m_pFile != nullptr, "Output file not open");

	auto pProcess{ std::make_unique<SNativeChildProcess>() };
	std::error_code error{};

#if defined(TGE_PLATFORM_WINDOWS)
	HANDLE const output{ (spec.pOutput != nullptr) ? spec.pOutput->m_pFile->file : INVALID_HANDLE_VALUE };
	std::filesystem::path const workingDir{ spec.workingDir.empty() ? std::filesystem::path{} : std::filesystem::absolute(spec.workingDir, error) };
	std::expected<std::filesystem::path, std::error_code> const program{ (error == std::error_code{}) ? ResolveProgram(spec.program, workingDir)
		: std::expected<std::filesystem::path, std::error_code>{ std::unexpected{ error } } };

	error = program.has_value() ? CreateChild(*pProcess, *program, spec.arguments, workingDir, output) : program.error();
#elif defined(TGE_PLATFORM_LINUX)
	error = Spawn(*pProcess, spec, (spec.pOutput != nullptr) ? spec.pOutput->m_pFile->descriptor : -1);
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	if (error == std::error_code{})
	{
		m_pProcess = std::move(pProcess);
	}

	return (error == std::error_code{}) ? std::expected<void, std::error_code>{} : std::unexpected{ error };
}

//////////////////////////////////////////////////////////////////////////
// A child still running keeps running; on Linux it stays a zombie once it ends, until the caller does.
void CChildProcess::Terminate()
{
#if defined(TGE_PLATFORM_WINDOWS)
	if (m_pProcess != nullptr && m_pProcess->process != nullptr)
	{
		CloseHandle(m_pProcess->process);
	}
#endif // defined(TGE_PLATFORM_WINDOWS)

	m_pProcess.reset();
}

//////////////////////////////////////////////////////////////////////////
std::optional<std::expected<SExitStatus, std::error_code>> CChildProcess::TryGetExit()
{
	TGE_ASSERT(m_pProcess != nullptr && !m_pProcess->hasEnded, "No running child to ask");

	std::optional<std::expected<SExitStatus, std::error_code>> result{};

#if defined(TGE_PLATFORM_WINDOWS)
	DWORD const waited{ WaitForSingleObject(m_pProcess->process, 0) };

	if (waited == WAIT_OBJECT_0)
	{
		DWORD code{ 0 };

		result = (GetExitCodeProcess(m_pProcess->process, &code) != FALSE)
			? std::expected<SExitStatus, std::error_code>{ SExitStatus{ static_cast<uint32_t>(code), {} } }
			: std::expected<SExitStatus, std::error_code>{ std::unexpected{ GetLastSystemError() } };
	}
	else if (waited == WAIT_FAILED)
	{
		result = std::expected<SExitStatus, std::error_code>{ std::unexpected{ GetLastSystemError() } };
	}

	if (result.has_value())
	{
		CloseHandle(m_pProcess->process);
		m_pProcess->process = nullptr;
	}
#elif defined(TGE_PLATFORM_LINUX)
	// This pid alone: waitpid(-1) would reap children that other code in the process is waiting for.
	int status{ 0 };
	pid_t waited{ ::waitpid(m_pProcess->pid, &status, WNOHANG) };

	while (waited < 0 && errno == EINTR)
	{
		waited = ::waitpid(m_pProcess->pid, &status, WNOHANG);
	}

	if (waited == m_pProcess->pid)
	{
		SExitStatus exit{};

		if (WIFEXITED(status))
		{
			exit.code = static_cast<uint32_t>(WEXITSTATUS(status));
		}
		else if (WIFSIGNALED(status))
		{
			exit.signal = GetSignalName(WTERMSIG(status));
		}

		result = std::expected<SExitStatus, std::error_code>{ std::move(exit) };
	}
	else if (waited < 0)
	{
		result = std::expected<SExitStatus, std::error_code>{ std::unexpected{ GetLastSystemError() } };
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	m_pProcess->hasEnded = result.has_value();

	return result;
}

//////////////////////////////////////////////////////////////////////////
uint32_t CChildProcess::GetId() const
{
	uint32_t id{ 0 };

	if (m_pProcess != nullptr)
	{
#if defined(TGE_PLATFORM_WINDOWS)
		id = static_cast<uint32_t>(m_pProcess->id);
#elif defined(TGE_PLATFORM_LINUX)
		id = static_cast<uint32_t>(m_pProcess->pid);
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
	}

	return id;
}
} // namespace Tge::Process
