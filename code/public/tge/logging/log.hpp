#pragma once

#include <tge/logging/log_message.hpp>
#include <tge/non_copyable.hpp>
#include <tge/color.hpp>
#include <cstdint>
#include <deque>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Tge::Logging
{
struct SListener final
{
	void*              key{ nullptr };
	LogMessageCallback callback;
	EMessageFormat     format{ EMessageFormat::Formatted };
};

class CLogSystem;
CLogSystem& GetLogSystem();

class CLog final : private SNoCopyNoMove
{
public:

	explicit CLog(std::string_view name, SColor const& color = SColor{});
	~CLog();

	// Logging methods with explicit target
	template<typename... Args>
	void Error(ETarget target, std::format_string<Args...> fmt, Args&&... args) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Error, target, std::format(fmt, std::forward<Args>(args)...));
#endif // TGE_LOGGING_ENABLED
	}

	template<typename... Args>
	void Warning(ETarget target, std::format_string<Args...> fmt, Args&&... args) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Warning, target, std::format(fmt, std::forward<Args>(args)...));
#endif // TGE_LOGGING_ENABLED
	}

	template<typename... Args>
	void Info(ETarget target, std::format_string<Args...> fmt, Args&&... args) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Info, target, std::format(fmt, std::forward<Args>(args)...));
#endif // TGE_LOGGING_ENABLED
	}

	// Convenience overloads defaulting to ETarget::All
	template<typename... Args>
	void Error(std::format_string<Args...> fmt, Args&&... args) const
	{
		Error(ETarget::All, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Warning(std::format_string<Args...> fmt, Args&&... args) const
	{
		Warning(ETarget::All, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void Info(std::format_string<Args...> fmt, Args&&... args) const
	{
		Info(ETarget::All, fmt, std::forward<Args>(args)...);
	}

	// String view variants with explicit target
	void Error(ETarget target, std::string_view message) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Error, target, std::string{message});
#endif // TGE_LOGGING_ENABLED
	}

	void Warning(ETarget target, std::string_view message) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Warning, target, std::string{message});
#endif // TGE_LOGGING_ENABLED
	}

	void Info(ETarget target, std::string_view message) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Info, target, std::string{message});
#endif // TGE_LOGGING_ENABLED
	}

	// String view variants defaulting to ETarget::All
	void Error(std::string_view message) const { Error(ETarget::All, message); }
	void Warning(std::string_view message) const { Warning(ETarget::All, message); }
	void Info(std::string_view message) const { Info(ETarget::All, message); }

	std::string_view GetName() const { return m_name; }
	void             SetName(std::string_view name) { m_name = name; }

	void RegisterListener(void* key, LogMessageCallback callback, EMessageFormat format = EMessageFormat::Formatted);
	void UnregisterListener(void* key);
	void FlushTo(void* key);
	void DispatchPending();

	void      SetLevelMask(ELogLevel level);
	ELogLevel GetLevelMask() const;

private:

	uint64_t    m_id{ 0 };
	std::string m_name;
	SColor      m_color;
	ELogLevel   m_levelMask{ ELogLevel::All };

	std::vector<SListener>                             m_listeners;
	mutable std::vector<std::shared_ptr<SLogMessage>> m_pendingDispatch;
	mutable std::deque<std::shared_ptr<SLogMessage>>  m_history;

	void Write(ELogLevel level, ETarget target, std::string message) const;
};
} // namespace Tge::Logging
