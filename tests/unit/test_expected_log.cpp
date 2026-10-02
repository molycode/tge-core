#include <tge/logging/log.hpp>
#include <tge/logging/log_level.hpp>
#include <tge/logging/log_system.hpp>
#include <tge/testing/expected_log.hpp>

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

namespace
{
using Tge::Logging::CLog;
using Tge::Logging::ELogLevel;
using Tge::Logging::ETarget;
using Tge::Logging::ETimestampMode;
using Tge::Logging::GetLogSystem;
using Tge::Testing::CExpectedLog;

// Each channel is declared before its guard: a CLog destroyed first takes its undispatched messages with it.
class CExpectedLogTest : public testing::Test
{
protected:

	void SetUp() override
	{
		// Never terminated: that would unregister every static channel for the rest of the binary.
		GetLogSystem().Initialize("test", "", "", ETimestampMode::Elapsed);
	}
};

//////////////////////////////////////////////////////////////////////////
void ExpectAWarningThatNeverComes()
{
	CLog const log{ "test_expected_missing" };
	CExpectedLog const expected{ "test_expected_missing", 1, 0 };
}

//////////////////////////////////////////////////////////////////////////
void LogOneErrorTooMany()
{
	CLog const log{ "test_expected_extra" };
	CExpectedLog const expected{ "test_expected_extra", 0, 1 };

	log.Error("the expected error");
	log.Error("one error too many");
}

//////////////////////////////////////////////////////////////////////////
void GuardAnUnknownChannel()
{
	CExpectedLog const expected{ "test_expected_no_such_channel", 0, 0 };
}

//////////////////////////////////////////////////////////////////////////
void NestTwoGuards()
{
	CLog const log{ "test_expected_nested" };
	CExpectedLog const outer{ "test_expected_nested", 0, 0 };
	CExpectedLog const inner{ "test_expected_nested", 0, 0 };
}

//////////////////////////////////////////////////////////////////////////
void GuardAMaskedChannel()
{
	CLog const log{ "test_expected_masked" };

	GetLogSystem().SetLogLevel("test_expected_masked", ELogLevel::Info);

	CExpectedLog const expected{ "test_expected_masked", 0, 0 };
}
} // namespace

TEST_F(CExpectedLogTest, ExactCountsPass)
{
	CLog const log{ "test_expected_counted" };
	CExpectedLog const expected{ "test_expected_counted", 1, 2 };

	log.Warning("the expected warning");
	log.Error("the first expected error");
	log.Error("the second expected error");
}

TEST_F(CExpectedLogTest, MissingWarningFails)
{
	EXPECT_NONFATAL_FAILURE(ExpectAWarningThatNeverComes(), "Warnings logged on channel 'test_expected_missing'");
}

TEST_F(CExpectedLogTest, OneErrorTooManyFails)
{
	EXPECT_NONFATAL_FAILURE(LogOneErrorTooMany(), "Errors logged on channel 'test_expected_extra'");
}

TEST_F(CExpectedLogTest, OtherChannelWarningIsNotCounted)
{
	CLog const own{ "test_expected_own" };
	CLog const other{ "test_expected_other" };
	CExpectedLog const expected{ "test_expected_own", 0, 0 };

	other.Warning("another channel's warning");
}

TEST_F(CExpectedLogTest, OwnChannelInfoIsNotCounted)
{
	CLog const log{ "test_expected_info" };
	CExpectedLog const expected{ "test_expected_info", 0, 0 };

	log.Info("this channel's info");
}

TEST_F(CExpectedLogTest, UnknownChannelFailsOnce)
{
	EXPECT_NONFATAL_FAILURE(GuardAnUnknownChannel(), "No log channel named 'test_expected_no_such_channel'");
}

TEST_F(CExpectedLogTest, NestedGuardFailsOnce)
{
	EXPECT_NONFATAL_FAILURE(NestTwoGuards(), "do not nest");
}

TEST_F(CExpectedLogTest, MaskedChannelFailsOnce)
{
	EXPECT_NONFATAL_FAILURE(GuardAMaskedChannel(), "masks Warnings or Errors");
}

// A guard that left the terminal off would mute the rest of the binary.
TEST_F(CExpectedLogTest, TargetsAreRestored)
{
	CLog const log{ "test_expected_restored" };
	ETarget const before{ GetLogSystem().GetEnabledTargets() };

	{
		CExpectedLog const expected{ "test_expected_restored", 0, 0 };
	}

	EXPECT_EQ(GetLogSystem().GetEnabledTargets(), before);
}
