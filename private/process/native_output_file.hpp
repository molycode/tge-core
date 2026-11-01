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
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Process
{
struct SNativeOutputFile final
{
#if defined(TGE_PLATFORM_WINDOWS)
	HANDLE file{ INVALID_HANDLE_VALUE };
#elif defined(TGE_PLATFORM_LINUX)
	int descriptor{ -1 };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
};
} // namespace Tge::Process
