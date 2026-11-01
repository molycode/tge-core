#include <tge/process/get_program_path.hpp>
#include <tge/platform.hpp>
#include <utility>

#if defined(TGE_PLATFORM_WINDOWS)
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Process
{
#if defined(TGE_PLATFORM_WINDOWS)
namespace
{
// Windows' longest path in UTF-16 units with its terminator, so a single call is never cut short.
constexpr DWORD MaxPathLength{ 32768 };
} // namespace
#endif // defined(TGE_PLATFORM_WINDOWS)

//////////////////////////////////////////////////////////////////////////
std::expected<std::filesystem::path, std::error_code> GetProgramPath()
{
#if defined(TGE_PLATFORM_WINDOWS)
	std::wstring name{};

	name.resize(MaxPathLength);

	DWORD const length{ ::GetModuleFileNameW(nullptr, name.data(), MaxPathLength) };
	bool const isWhole{ length != 0 && length < MaxPathLength };
	std::error_code const error{ static_cast<int>(isWhole ? ERROR_SUCCESS : ::GetLastError()), std::system_category() };

	name.resize(isWhole ? length : 0);

	std::filesystem::path path{ std::move(name) };
#elif defined(TGE_PLATFORM_LINUX)
	std::error_code error{};
	std::filesystem::path path{ std::filesystem::read_symlink("/proc/self/exe", error) };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return (error.value() == 0) ? std::expected<std::filesystem::path, std::error_code>{ std::move(path) } : std::unexpected{ error };
}
} // namespace Tge::Process
