#include "loggers.hpp"
#include <tge/assert.hpp>

namespace Tge
{
//////////////////////////////////////////////////////////////////////////
void FatalError(char const* message, char const* file, int line)
{
	gLog.Error("\n=== FATAL ERROR ===");
	gLog.Error("Location: {}:{}", file, line);
	gLog.Error("Message: {}", message);
	gLog.Error("===================\n");

	// Generate trap instruction for debugger-friendly crash
	#if defined(_MSC_VER)
		__debugbreak();  // MSVC intrinsic
	#elif defined(__GNUC__) || defined(__clang__)
		__builtin_trap();  // GCC/Clang intrinsic
	#else
		std::abort();  // Fallback for unknown compilers
	#endif
}
} // namespace Tge
