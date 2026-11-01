#include <tge/process/is_runnable.hpp>
#include <tge/platform.hpp>
#include <system_error>

#if defined(TGE_PLATFORM_WINDOWS)
#include "startable_extension.hpp"
#elif defined(TGE_PLATFORM_LINUX)
#include <unistd.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Process
{
//////////////////////////////////////////////////////////////////////////
bool IsRunnable(std::filesystem::path const& path)
{
	std::error_code error{};
	bool const isFile{ std::filesystem::is_regular_file(path, error) };

#if defined(TGE_PLATFORM_WINDOWS)
	return isFile && IsStartableExtension(path.extension().native());
#elif defined(TGE_PLATFORM_LINUX)
	return isFile && ::access(path.c_str(), X_OK) == 0;
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
}
} // namespace Tge::Process
