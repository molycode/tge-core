#include <tge/command/args.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using namespace Tge;

//////////////////////////////////////////////////////////////////////////
TEST(CommandArgs, ParseFloatAcceptsAWellFormedNumber)
{
	float value{ 0.0f };

	EXPECT_TRUE(Command::ParseFloat("1.5", value));
	EXPECT_FLOAT_EQ(value, 1.5f);
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandArgs, ParseFloatAcceptsANegativeNumber)
{
	float value{ 0.0f };

	EXPECT_TRUE(Command::ParseFloat("-0.25", value));
	EXPECT_FLOAT_EQ(value, -0.25f);
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandArgs, ParseFloatRejectsTrailingCharacters)
{
	float value{ 0.0f };

	EXPECT_FALSE(Command::ParseFloat("1.5abc", value));
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandArgs, ParseFloatRejectsTextThatIsNotANumber)
{
	float value{ 0.0f };

	EXPECT_FALSE(Command::ParseFloat("off", value));
	EXPECT_FALSE(Command::ParseFloat("", value));
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandArgs, ParseIndexAcceptsADecimalIndex)
{
	uint32_t value{ 0 };

	EXPECT_TRUE(Command::ParseIndex("42", value));
	EXPECT_EQ(value, 42u);
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandArgs, ParseIndexRejectsANegativeValue)
{
	uint32_t value{ 0 };

	EXPECT_FALSE(Command::ParseIndex("-1", value));
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandArgs, ParseIndexRejectsTrailingCharacters)
{
	uint32_t value{ 0 };

	EXPECT_FALSE(Command::ParseIndex("42x", value));
	EXPECT_FALSE(Command::ParseIndex("", value));
}
