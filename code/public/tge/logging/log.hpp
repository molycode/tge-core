#pragma once

#include <tge/logging/log_level.hpp>
#include <tge/color.hpp>
#include <cstdint>
#include <format>
#include <string_view>

namespace Tge::Logging
{
class CLogSystem;
CLogSystem& GetLogSystem();

class CLog final
{
public:

	explicit CLog(std::string_view name, SColor const& color = SColor{});

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
		Write(ELogLevel::Error, target, message);
#endif // TGE_LOGGING_ENABLED
	}

	void Warning(ETarget target, std::string_view message) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Warning, target, message);
#endif // TGE_LOGGING_ENABLED
	}

	void Info(ETarget target, std::string_view message) const
	{
#ifdef TGE_LOGGING_ENABLED
		Write(ELogLevel::Info, target, message);
#endif // TGE_LOGGING_ENABLED
	}

	// String view variants defaulting to ETarget::All
	void Error(std::string_view message) const { Error(ETarget::All, message); }
	void Warning(std::string_view message) const { Warning(ETarget::All, message); }
	void Info(std::string_view message) const { Info(ETarget::All, message); }

	std::string_view GetName() const;

private:

	uint64_t m_id = 0;

	void Write(ELogLevel level, ETarget target, std::string_view message) const;
};
} // namespace Tge::Logging
