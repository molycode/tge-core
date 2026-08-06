#include <gtest/gtest.h>

#include <tge/logging/log.hpp>
#include <tge/logging/log_system.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace
{
using Tge::Logging::CLog;
using Tge::Logging::EHistory;
using Tge::Logging::ELogLevel;
using Tge::Logging::EMessageFormat;
using Tge::Logging::ETimestampMode;
using Tge::Logging::GetLogSystem;
using Tge::Logging::SLogMessage;

class CLogListenerTest : public testing::Test
{
protected:

	void SetUp() override
	{
		// Empty logs/config directories keep the suite from writing a file or reading the shipped channel levels.
		GetLogSystem().Initialize("test", "", "", ETimestampMode::Elapsed);
	}

	void TearDown() override
	{
		GetLogSystem().UnregisterListener(this);
		GetLogSystem().DispatchListeners();
	}

	void Listen(EHistory history)
	{
		GetLogSystem().RegisterListener(this, [this](SLogMessage const& message)
		{
			m_delivered.emplace_back(message.message);
		}, EMessageFormat::Raw, history);
	}

	size_t CountDelivered(std::string_view message) const
	{
		return static_cast<size_t>(std::count(m_delivered.begin(), m_delivered.end(), message));
	}

	std::vector<std::string> m_delivered;
};

// A message logged before the listener exists is queued for dispatch AND kept in the history the listener
// asks to be flushed. It must still arrive exactly once.
TEST_F(CLogListenerTest, HistoryFlushDeliversAQueuedMessageOnce)
{
	CLog log{ "test_flush_once" };

	log.Info("first");
	log.Info("second");

	Listen(EHistory::Flush);
	GetLogSystem().DispatchListeners();

	EXPECT_EQ(CountDelivered("first"), 1u);
	EXPECT_EQ(CountDelivered("second"), 1u);
}

// The flush merges every channel's history by timestamp, so a listener attaching mid-boot sees the log in the
// order it was written rather than grouped per channel.
TEST_F(CLogListenerTest, HistoryFlushDeliversAcrossChannelsInLoggedOrder)
{
	CLog first{ "test_order_first" };
	CLog second{ "test_order_second" };

	first.Info("one");
	second.Info("two");
	first.Info("three");

	Listen(EHistory::Flush);
	GetLogSystem().DispatchListeners();

	std::vector<std::string> const expected{ "one", "two", "three" };
	EXPECT_EQ(m_delivered, expected);
}

// Skipping the history means skipping all of it — including the tail that happens to be queued for a
// dispatch that has not run yet.
TEST_F(CLogListenerTest, HistorySkipDeliversNothingLoggedBeforeRegistration)
{
	CLog log{ "test_skip_backlog" };

	log.Info("before");

	Listen(EHistory::Skip);
	GetLogSystem().DispatchListeners();

	EXPECT_EQ(CountDelivered("before"), 0u);
}

// A message logged after the listener attaches reaches it through the dispatch alone.
TEST_F(CLogListenerTest, LiveMessagesAreDeliveredOnce)
{
	CLog log{ "test_live_once" };

	Listen(EHistory::Flush);

	log.Info("live");
	GetLogSystem().DispatchListeners();
	GetLogSystem().DispatchListeners();

	EXPECT_EQ(CountDelivered("live"), 1u);
}
} // namespace
