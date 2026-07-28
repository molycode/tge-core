#pragma once

#include <tge/config.hpp>
#include <tge/logging/log_level.hpp>
#include <tge/logging/log_system.hpp>
#include <tge/non_copyable.hpp>

#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <utility>

namespace Tge::Testing
{
// A rejection test feeds a module corrupt input on purpose, and every module's own rule is that a failure
// path logs an Error. Those expected errors are indistinguishable from a genuine one raised elsewhere in the
// run, so the test that expects them swallows them: anything red left on the console then means something.
//
// RAII rather than the Initialize/Terminate pattern because a failing ASSERT_* returns straight out of the
// test body — an explicit restore call would be skipped, leaving the channel muted for the whole process.
//
// Inert without logging: no channel exists to mute, so the drift check below cannot run in that build.
class CExpectedLogErrors final : private SNoCopyNoMove
{
public:

	explicit CExpectedLogErrors(std::string_view channel)
		: m_channel{ channel }
		, m_savedLevel{ Logging::GetLogSystem().GetLogLevel(channel) }
	{
#ifdef TGE_LOGGING_ENABLED
		Logging::ELogLevel const withoutErrors{ static_cast<Logging::ELogLevel>(
			std::to_underlying(m_savedLevel) & ~std::to_underlying(Logging::ELogLevel::Error)) };

		// An unknown channel would mute nothing at all, and the test would still look like it passed quietly.
		EXPECT_TRUE(Logging::GetLogSystem().SetLogLevel(m_channel, withoutErrors))
			<< "No log channel named '" << channel << "' — this guard is silently doing nothing";
#endif // TGE_LOGGING_ENABLED
	}

	~CExpectedLogErrors()
	{
#ifdef TGE_LOGGING_ENABLED
		Logging::GetLogSystem().SetLogLevel(m_channel, m_savedLevel);
#endif // TGE_LOGGING_ENABLED
	}

private:

	std::string        m_channel;
	Logging::ELogLevel m_savedLevel{ Logging::ELogLevel::All };
};
} // namespace Tge::Testing
