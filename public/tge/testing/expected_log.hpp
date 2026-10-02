#pragma once

#include <tge/config.hpp>
#include <tge/logging/log_level.hpp>
#include <tge/logging/log_message.hpp>
#include <tge/logging/log_system.hpp>
#include <tge/non_copyable.hpp>

#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Tge::Testing
{
// Counts a channel's Warnings and Errors in its scope instead of printing them; the counts must match exactly.
//
// The terminal is off for the whole log system meanwhile, and what the guard does not count it prints at scope end.
// So the test thread must be the only one dispatching listeners, and other threads must not log while the guard is
// built and must be joined before its scope ends.
//
// Inert without logging: nothing could be counted in that build.
class CExpectedLog final : private SNoCopyNoMove
{
public:

	CExpectedLog(std::string_view channel, uint32_t numWarnings, uint32_t numErrors)
#ifdef TGE_LOGGING_ENABLED
		: m_channel{ channel }
		, m_numExpectedWarnings{ numWarnings }
		, m_numExpectedErrors{ numErrors }
#endif // TGE_LOGGING_ENABLED
	{
#ifdef TGE_LOGGING_ENABLED
		Logging::CLogSystem& logSystem{ Logging::GetLogSystem() };
		std::vector<std::string_view> const channels{ logSystem.GetChannelNames() };
		Logging::ELogLevel const counted{ Logging::ELogLevel::Warning | Logging::ELogLevel::Error };

		// One failure at most: a guard that cannot count must not also report the counts it never saw.
		if (!logSystem.IsInitialized())
		{
			ADD_FAILURE() << "CExpectedLog needs an initialized log system: before that, no listener sees a message";
		}
		else if (m_isAnyArmed)
		{
			ADD_FAILURE() << "CExpectedLog guards do not nest";
		}
		else if (std::ranges::find(channels, channel) == channels.end())
		{
			ADD_FAILURE() << "No log channel named '" << channel << "': nothing could ever be counted";
		}
		else if ((logSystem.GetLogLevel(channel) & counted) != counted)
		{
			ADD_FAILURE() << "Channel '" << channel << "' masks Warnings or Errors, so none would arrive";
		}
		else
		{
			m_savedTargets = logSystem.GetEnabledTargets();
			m_isEchoing = (m_savedTargets & Logging::ETarget::Terminal) != Logging::ETarget::None;

			Logging::ETarget const silenced{ static_cast<Logging::ETarget>(
				std::to_underlying(m_savedTargets | Logging::ETarget::Listeners) & ~std::to_underlying(Logging::ETarget::Terminal)) };

			logSystem.SetEnabledTargets(silenced);
			logSystem.RegisterListener(this, [this](Logging::SLogMessage const& message)
			{
				Count(message);
			}, Logging::EMessageFormat::Formatted, Logging::EHistory::Skip);

			m_isAnyArmed = true;
			m_isArmed = true;
		}
#endif // TGE_LOGGING_ENABLED
	}

	~CExpectedLog()
	{
#ifdef TGE_LOGGING_ENABLED
		if (m_isArmed)
		{
			Logging::CLogSystem& logSystem{ Logging::GetLogSystem() };

			logSystem.DispatchListeners();
			logSystem.UnregisterListener(this);
			logSystem.SetEnabledTargets(m_savedTargets);
			m_isAnyArmed = false;

			EXPECT_EQ(m_numWarnings, m_numExpectedWarnings) << "Warnings logged on channel '" << m_channel << "'";
			EXPECT_EQ(m_numErrors, m_numExpectedErrors) << "Errors logged on channel '" << m_channel << "'";
		}
#endif // TGE_LOGGING_ENABLED
	}

private:

#ifdef TGE_LOGGING_ENABLED
	void Count(Logging::SLogMessage const& message)
	{
		bool const isOwnChannel{ message.channelName == m_channel };

		if (isOwnChannel && message.level == Logging::ELogLevel::Warning)
		{
			++m_numWarnings;
		}
		else if (isOwnChannel && message.level == Logging::ELogLevel::Error)
		{
			++m_numErrors;
		}
		else if (m_isEchoing)
		{
			Echo(message);
		}
	}

	// Error to stderr and the rest to stdout, coloured as the terminal colours them.
	static void Echo(Logging::SLogMessage const& message)
	{
		if (message.level == Logging::ELogLevel::Error)
		{
			std::cerr << "\033[31m" << message.message << "\033[0m\n" << std::flush;
		}
		else if (message.level == Logging::ELogLevel::Warning)
		{
			std::cout << "\033[33m" << message.message << "\033[0m\n" << std::flush;
		}
		else
		{
			std::cout << message.message << '\n' << std::flush;
		}
	}

	std::string      m_channel;
	uint32_t         m_numExpectedWarnings{ 0 };
	uint32_t         m_numExpectedErrors{ 0 };
	uint32_t         m_numWarnings{ 0 };
	uint32_t         m_numErrors{ 0 };
	Logging::ETarget m_savedTargets{ Logging::ETarget::All };
	bool             m_isEchoing{ false };
	bool             m_isArmed{ false };

	static inline bool m_isAnyArmed{ false };
#endif // TGE_LOGGING_ENABLED
};
} // namespace Tge::Testing
