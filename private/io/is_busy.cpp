#include <tge/io/is_busy.hpp>
#include <tge/platform.hpp>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
// Access denied belongs here: Windows refuses to rename a folder with it while a file inside is open. Linux never
// refuses a file for being open.
bool IsBusy(std::error_code const& error)
{
#if defined(TGE_PLATFORM_WINDOWS)
	return error.category() == std::system_category()
		&& (error.value() == ERROR_ACCESS_DENIED || error.value() == ERROR_SHARING_VIOLATION || error.value() == ERROR_LOCK_VIOLATION);
#elif defined(TGE_PLATFORM_LINUX)
	return false;
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}
} // namespace Tge::IO
