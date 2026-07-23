#pragma once

#include <tge/config.hpp>

#include <tge/logging/log_message.hpp>
#include <tge/non_copyable.hpp>
#include <atomic>
#include <string>
#include <string_view>
#include <vector>

namespace Tge
{
namespace Logging
{
class CLog;

enum class EHistory
{
	Skip,
	Flush
};

class CLogSystem final : private SNoCopyNoMove
{
public:

	CLogSystem() = default;
	~CLogSystem() = default;

	void Initialize(std::string_view prefix = "tge", std::string_view logsDir = "logs", std::string_view configDir = "configs", ETimestampMode timestampMode = ETimestampMode::Elapsed);
	void Terminate();
	bool IsInitialized() const;
	ETimestampMode GetTimestampMode() const;

	uint64_t Register(std::string_view name);

	bool SetLogLevel(std::string_view channelName, ELogLevel level);
	void SetAllLogLevels(ELogLevel level);
	ELogLevel GetLogLevel(std::string_view channelName) const;

	void SetEnabledTargets(ETarget targets);
	ETarget GetEnabledTargets() const;

	std::vector<std::string_view> GetChannelNames() const;

	std::string_view GetChannelNameById(uint64_t channelId) const;

	void RegisterListener(void* key, LogMessageCallback callback, EMessageFormat format = EMessageFormat::Formatted, EHistory history = EHistory::Skip);
	void UnregisterListener(void* key);

	void DispatchListeners();

private:

#ifdef TGE_LOGGING_ENABLED
	bool                 m_initialized{ false };
	ETimestampMode       m_timestampMode{ ETimestampMode::Elapsed };
	std::atomic<ETarget> m_enabledTargets{ ETarget::All };
#endif // TGE_LOGGING_ENABLED
};

CLogSystem& GetLogSystem();
} // namespace Logging
} // namespace Tge
