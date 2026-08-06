#pragma once

// Always use '#if defined(TGE_PLATFORM_*)' — never '#if TGE_PLATFORM_*'.
// An undefined macro silently evaluates to 0, masking misconfiguration.
//
// To add a platform: add its compiler macro detection below.

#if defined(__ANDROID__)
#	define TGE_PLATFORM_ANDROID
#elif defined(_WIN32)
#	define TGE_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#	include <TargetConditionals.h>
#	if TARGET_OS_IOS
#		define TGE_PLATFORM_IOS
#		define TGE_PLATFORM_APPLE
#	elif TARGET_OS_OSX
#		define TGE_PLATFORM_MACOS
#		define TGE_PLATFORM_APPLE
#	else
#		error "Unsupported Apple platform. Add detection to <tge/platform.hpp>."
#	endif
#elif defined(__linux__)
#	define TGE_PLATFORM_LINUX
#else
#	error "Unknown platform. Add compiler macro detection to <tge/platform.hpp>."
#endif
