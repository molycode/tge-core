#pragma once

#include <tge/logging/log_level.hpp>
#include <tge/color.hpp>

#ifdef TGE_LOGGING_ENABLED
#include <cstdint>
#include <format>
#else
#include <string_view>
#endif // TGE_LOGGING_ENABLED

namespace Tge::Logging
{
class CLogSystem;
CLogSystem& GetLogSystem();

class CLog final
{
public:

#ifdef TGE_LOGGING_ENABLED
	explicit CLog(std::string_view name, SColor const& color = SColor{});
#else
	explicit CLog(std::string_view name, SColor const& color = SColor{}) {}
#endif // TGE_LOGGING_ENABLED

#ifdef TGE_LOGGING_ENABLED

	// Logging methods with explicit target
	template<typename... Args>
	void Error(ETarget target, std::format_string<Args...> fmt, Args&&... args) const
	{
		Write(ELogLevel::Error, target, std::format(fmt, std::forward<Args>(args)...));
	}

	template<typename... Args>
	void Warning(ETarget target, std::format_string<Args...> fmt, Args&&... args) const
	{
		Write(ELogLevel::Warning, target, std::format(fmt, std::forward<Args>(args)...));
	}

	template<typename... Args>
	void Info(ETarget target, std::format_string<Args...> fmt, Args&&... args) const
	{
		Write(ELogLevel::Info, target, std::format(fmt, std::forward<Args>(args)...));
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
	void Error(ETarget target, std::string_view message) const { Write(ELogLevel::Error, target, message); }
	void Warning(ETarget target, std::string_view message) const { Write(ELogLevel::Warning, target, message); }
	void Info(ETarget target, std::string_view message) const { Write(ELogLevel::Info, target, message); }

	// String view variants defaulting to ETarget::All
	void Error(std::string_view message) const { Error(ETarget::All, message); }
	void Warning(std::string_view message) const { Warning(ETarget::All, message); }
	void Info(std::string_view message) const { Info(ETarget::All, message); }

	std::string_view GetName() const;

private:

	uint64_t m_id = 0;

	void Write(ELogLevel level, ETarget target, std::string_view message) const;

#else // TGE_LOGGING_ENABLED

	// No-op implementations when logging is disabled
	template<typename... Args>
	void Error(ETarget, Args&&...) const {}

	template<typename... Args>
	void Warning(ETarget, Args&&...) const {}

	template<typename... Args>
	void Info(ETarget, Args&&...) const {}

	template<typename... Args>
	void Error(Args&&...) const {}

	template<typename... Args>
	void Warning(Args&&...) const {}

	template<typename... Args>
	void Info(Args&&...) const {}

	void Error(ETarget, std::string_view) const {}
	void Warning(ETarget, std::string_view) const {}
	void Info(ETarget, std::string_view) const {}

	void Error(std::string_view) const {}
	void Warning(std::string_view) const {}
	void Info(std::string_view) const {}

	std::string_view GetName() const { return ""; }

#endif // TGE_LOGGING_ENABLED
};
} // namespace Tge::Logging
