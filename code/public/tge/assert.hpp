#pragma once

#include <cstdio>

namespace Tge
{
// Header-only + stderr so assert carries no link dependency (no static-lib cycle) and keeps the abort off the logger it may be reporting on.
[[noreturn]] inline void FatalError(char const* message, char const* file, int line)
{
	std::fprintf(stderr,
		"\n=== FATAL ERROR ===\n"
		"Location: %s:%d\n"
		"Message: %s\n"
		"===================\n",
		file, line, message);

	#if defined(_MSC_VER)
		__debugbreak();
	#elif defined(__GNUC__) || defined(__clang__)
		__builtin_trap();
	#else
		std::abort();
	#endif
}
} // namespace Tge

// Fatal error macro - use for unrecoverable errors
#ifdef TGE_DEBUG_ENABLED
	#define TGE_FATAL(msg) ::Tge::FatalError(msg, __FILE__, __LINE__)
#else
	#if defined(_MSC_VER)
		#define TGE_FATAL(msg) __debugbreak()
	#elif defined(__GNUC__) || defined(__clang__)
		#define TGE_FATAL(msg) __builtin_trap()
	#else
		#define TGE_FATAL(msg) std::abort()
	#endif // _MSC_VER
#endif // TGE_DEBUG_ENABLED

// Unreachable code path macro - use for code that should never execute
#ifdef TGE_DEBUG_ENABLED
	#define TGE_UNREACHABLE(msg) TGE_FATAL(msg)
#else
	#if defined(__GNUC__) || defined(__clang__)
		#define TGE_UNREACHABLE(msg) __builtin_unreachable()
	#elif defined(_MSC_VER)
		#define TGE_UNREACHABLE(msg) __assume(0)
	#else
		#define TGE_UNREACHABLE(msg) std::abort()
	#endif // __GNUC__ || __clang__
#endif // TGE_DEBUG_ENABLED

// Assert macro - use for debug-time validation (disabled in release builds)
#ifdef TGE_DEBUG_ENABLED
	#define TGE_ASSERT(condition, msg) \
		do { \
			if (!(condition)) { \
				::Tge::FatalError("Assertion failed: " msg, __FILE__, __LINE__); \
			} \
		} while (0)
#else
	#define TGE_ASSERT(condition, msg) ((void)0)
#endif // TGE_DEBUG_ENABLED
