#include <gtest/gtest.h>

#include <tge/logging/log.hpp>
#include <tge/logging/log_system.hpp>
#include <tge/platform.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
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

	bool IsDeliveredStartingWith(std::string_view start) const
	{
		return std::ranges::any_of(m_delivered, [start](std::string const& message) { return message.starts_with(start); });
	}

	// The log system writes into `folder`, then goes back to writing no file, so no later test logs there.
	static size_t CountLogFiles(std::filesystem::path const& folder)
	{
		GetLogSystem().Terminate();
		GetLogSystem().Initialize("test", folder.string(), "", ETimestampMode::Elapsed);
		GetLogSystem().Terminate();
		GetLogSystem().Initialize("test", "", "", ETimestampMode::Elapsed);

		std::error_code error;
		size_t numFiles{ 0 };

		for (std::filesystem::directory_entry const& entry : std::filesystem::directory_iterator{ folder, error })
		{
			std::string const name{ entry.path().filename().string() };

			if (name.starts_with("test_") && name.ends_with(".log"))
			{
				++numFiles;
			}
		}

		return numFiles;
	}

	static std::filesystem::path FreshFolder(std::string_view name)
	{
		std::filesystem::path const folder{ std::filesystem::path{ testing::TempDir() } / name };

		std::filesystem::remove_all(folder);

		return folder;
	}

	std::vector<std::string> m_delivered;
};

// A program's channels are built once, before the log system first starts, and outlive any restart of it.
// Negative control: Terminate forgetting the channels it knows, and nothing the channel logs afterwards arrives.
TEST_F(CLogListenerTest, AChannelStillDeliversAfterARestart)
{
	CLog log{ "test_restart" };

	GetLogSystem().Terminate();
	GetLogSystem().Initialize("test", "", "", ETimestampMode::Elapsed);

	Listen(EHistory::Skip);
	log.Info("after the restart");
	GetLogSystem().DispatchListeners();

	EXPECT_EQ(CountDelivered("after the restart"), 1u);
}

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

// The logs folder's path joins the file name as it is, never as part of a format.
// Negative control: format the whole path with strftime again, and the file goes to a folder named after the year.
TEST_F(CLogListenerTest, APercentSignInTheLogsFolderIsTakenLiterally)
{
	std::filesystem::path const folder{ FreshFolder("tge_log_percent") / "logs_%Y" };

	EXPECT_EQ(CountLogFiles(folder), 1u);
}

#if defined(TGE_PLATFORM_LINUX)
// Windows refuses such a path itself unless long paths are enabled.
// Negative control: build the name in a 256-byte buffer again, and strftime leaves it unwritten.
TEST_F(CLogListenerTest, ALogsFolderPastTwoHundredFiftySixCharactersStillGetsItsFile)
{
	std::filesystem::path folder{ FreshFolder("tge_log_long") };

	for (int i = 0; i < 6; ++i)
	{
		folder /= std::string(50, static_cast<char>('a' + i));
	}

	EXPECT_EQ(CountLogFiles(folder), 1u);
}
#endif // defined(TGE_PLATFORM_LINUX)

// Negative control: create the folder with the throwing overload again, and Initialize throws.
TEST_F(CLogListenerTest, ALogsFolderThatCannotBeMadeLeavesTheConsoleAndSaysSo)
{
	std::filesystem::path const blocker{ FreshFolder("tge_log_blocker") };
	std::ofstream{ blocker } << "a file, not a folder";

	GetLogSystem().Terminate();
	GetLogSystem().Initialize("test", (blocker / "logs").string(), "", ETimestampMode::Elapsed);
	Listen(EHistory::Flush);
	GetLogSystem().DispatchListeners();

	EXPECT_TRUE(IsDeliveredStartingWith("Cannot create the logs folder"));
	EXPECT_TRUE(GetLogSystem().IsInitialized());
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
