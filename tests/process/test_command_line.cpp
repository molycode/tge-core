#include "command_line.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace Tge::Process;

//////////////////////////////////////////////////////////////////////////
// Kingpin splits its command line on whitespace and keeps quotes, so a plain argument must go as it is.
TEST(CommandLine, PlainArgumentsGoAsTheyAre)
{
	std::vector<std::string> const arguments{ "+set", "password", "s3cret", "+connect", "203.0.113.7:31510" };

	EXPECT_EQ(BuildCommandLine("C:\\Games\\Kingpin\\kingpin.exe", arguments),
		"\"C:\\Games\\Kingpin\\kingpin.exe\" +set password s3cret +connect 203.0.113.7:31510");
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandLine, SpacesAndEmptyArgumentsAreQuoted)
{
	std::vector<std::string> const arguments{ "with space", "" };

	EXPECT_EQ(BuildCommandLine("C:\\game.exe", arguments), "\"C:\\game.exe\" \"with space\" \"\"");
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandLine, TabsAndLineBreaksAreQuoted)
{
	std::vector<std::string> const arguments{ "a\tb", "c\nd", "e\vf" };

	EXPECT_EQ(BuildCommandLine("C:\\game.exe", arguments), "\"C:\\game.exe\" \"a\tb\" \"c\nd\" \"e\vf\"");
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandLine, QuotesAreEscaped)
{
	std::vector<std::string> const arguments{ "say \"hi\"" };

	EXPECT_EQ(BuildCommandLine("C:\\game.exe", arguments), "\"C:\\game.exe\" \"say \\\"hi\\\"\"");
}

//////////////////////////////////////////////////////////////////////////
TEST(CommandLine, BackslashesDoubleOnlyBeforeAQuote)
{
	std::vector<std::string> const arguments{ "C:\\plain", "C:\\a b\\", "a\\\"b" };

	EXPECT_EQ(BuildCommandLine("C:\\game.exe", arguments), "\"C:\\game.exe\" C:\\plain \"C:\\a b\\\\\" \"a\\\\\\\"b\"");
}
