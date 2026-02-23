#pragma once

#include <tge/logging/log_message.hpp>
#include <string_view>
#include <vector>

namespace Tge
{
struct SColor;

namespace Logging
{
class CLog;

class CLogSystem final
{
public:

	CLogSystem() = default;
	~CLogSystem() = default;

	void Initialize(std::string_view prefix = "tge");
	void Terminate();
	bool IsInitialized() const;

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

#ifdef TGE_LOGGING_ENABLED
	bool m_initialized = false;
#endif // TGE_LOGGING_ENABLED
};

CLogSystem& GetLogSystem();
} // namespace Logging
} // namespace Tge
