#include "threading/os_thread_name.hpp"

#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

namespace Tge::Threading
{
//////////////////////////////////////////////////////////////////////////
void SetOsThreadName(std::string_view name)
{
	std::wstring wideName{};
	int const numWide{ MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0) };

	wideName.resize(static_cast<size_t>(numWide));
	MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), wideName.data(), numWide);
	SetThreadDescription(GetCurrentThread(), wideName.c_str());
}
} // namespace Tge::Threading
