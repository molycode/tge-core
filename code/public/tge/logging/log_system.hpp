#pragma once

#include <tge/logging/log_level.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace Tge
{
struct SColor;

namespace Logging
{
class CLog;

struct SLogMessage final
{
	std::chrono::system_clock::time_point timestamp;
	uint64_t elapsedMs = 0;  // Milliseconds since application start
	ELogLevel level;
	ETarget target;
	std::string channelName;
	std::string message;
	uint8_t colorR = 255;
	uint8_t colorG = 255;
	uint8_t colorB = 255;
};

using LogMessageCallback = std::function<void(SLogMessage const& message)>;

class CLogSystem final
{
public:

	CLogSystem() = default;
	~CLogSystem() = default;

#ifdef TGE_LOGGING_ENABLED
	void Initialize();
	void Terminate();
	bool IsInitialized() const { return m_initialized; }

	uint64_t Register(std::string_view name, SColor const& color);

	bool SetLogLevel(std::string_view channelName, ELogLevel level);
	void SetAllLogLevels(ELogLevel level);
	ELogLevel GetLogLevel(std::string_view channelName) const;

	std::vector<std::string_view> GetChannelNames() const;

	void Write(uint64_t channelId, ELogLevel level, ETarget target, std::string_view message);

	std::string_view GetChannelNameById(uint64_t channelId) const;

	void RegisterListener(void* key, LogMessageCallback callback);
	void FlushTo(void* key);
	void UnregisterListener(void* key);

private:

	bool m_initialized = false;
#else
	void Initialize() {}
	void Terminate() {}
	bool IsInitialized() const { return false; }

	uint64_t Register(std::string_view, SColor const&) { return 0; }

	bool SetLogLevel(std::string_view, ELogLevel) { return false; }
	void SetAllLogLevels(ELogLevel) {}
	ELogLevel GetLogLevel(std::string_view) const { return ELogLevel::All; }

	std::vector<std::string_view> GetChannelNames() const { return {}; }

	void Write(uint64_t, ELogLevel, ETarget, std::string_view) {}

	std::string_view GetChannelNameById(uint64_t) const { return ""; }

	void RegisterListener(void*, LogMessageCallback) {}
	void FlushTo(void*) {}
	void UnregisterListener(void*) {}
#endif // TGE_LOGGING_ENABLED
};

CLogSystem& GetLogSystem();
} // namespace Logging
} // namespace Tge
