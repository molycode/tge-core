#pragma once

#include <cstdint>
#include <utility>

namespace Tge::Logging
{
enum class ELogLevel : uint8_t
{
	None    = 0,
	Error   = 1 << 0,  // Error messages
	Warning = 1 << 1,  // Warning messages
	Info    = 1 << 2,  // Informational messages
	All     = Error | Warning | Info  // All message types (default)
};

inline ELogLevel operator|(ELogLevel a, ELogLevel b)
{
	return static_cast<ELogLevel>(std::to_underlying(a) | std::to_underlying(b));
}

inline ELogLevel operator&(ELogLevel a, ELogLevel b)
{
	return static_cast<ELogLevel>(std::to_underlying(a) & std::to_underlying(b));
}

enum class ETarget : uint8_t
{
	None     = 0,
	Terminal = 1 << 0,  // stdout/stderr
	File     = 1 << 1,  // Log file
	Console  = 1 << 2,  // Application console (listener)
	All      = Terminal | File | Console
};

inline ETarget operator|(ETarget a, ETarget b)
{
	return static_cast<ETarget>(std::to_underlying(a) | std::to_underlying(b));
}

inline ETarget operator&(ETarget a, ETarget b)
{
	return static_cast<ETarget>(std::to_underlying(a) & std::to_underlying(b));
}
} // namespace Tge::Logging
