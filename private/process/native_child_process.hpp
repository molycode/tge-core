#pragma once

#include <tge/platform.hpp>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <sys/types.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Process
{
struct SNativeChildProcess final
{
#if defined(TGE_PLATFORM_WINDOWS)
	HANDLE process{ nullptr };
	DWORD id{ 0 };
#elif defined(TGE_PLATFORM_LINUX)
	pid_t pid{ 0 };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
	bool hasEnded{ false };
};
} // namespace Tge::Process
