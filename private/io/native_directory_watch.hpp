#pragma once

#include <tge/io/watch_scope.hpp>
#include <tge/platform.hpp>
#include <filesystem>

#if defined(TGE_PLATFORM_WINDOWS)
#include <vector>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <map>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::IO
{
struct SNativeDirectoryWatch final
{
	EWatchScope scope{ EWatchScope::Folder };
	std::filesystem::path root{};
#if defined(TGE_PLATFORM_WINDOWS)
	HANDLE folder{ INVALID_HANDLE_VALUE };
	OVERLAPPED overlapped{};
	std::vector<DWORD> buffer{};
	bool isReadPending{ false };
#elif defined(TGE_PLATFORM_LINUX)
	int descriptor{ -1 };
	int rootWatch{ -1 };
	std::map<int, std::filesystem::path> folders{};
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
};
} // namespace Tge::IO
