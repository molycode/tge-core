#pragma once

#include <tge/logging/log_level.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace Tge::Logging
{
struct SLogMessage final
{
	std::chrono::system_clock::time_point timestamp;
	uint64_t elapsedMs = 0;  // Milliseconds since application start
	ELogLevel level;
	ETarget target;
	std::string channelName;
	std::string message;
	std::string formattedTimestamp{};
	uint8_t colorR = 255;
	uint8_t colorG = 255;
	uint8_t colorB = 255;
};

using LogMessageCallback = std::function<void(SLogMessage const& message)>;
} // namespace Tge::Logging
