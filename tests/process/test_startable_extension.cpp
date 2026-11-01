#include "startable_extension.hpp"
#include <gtest/gtest.h>
#include <filesystem>

using namespace Tge::Process;

//////////////////////////////////////////////////////////////////////////
TEST(StartableExtension, ExeAndComInAnyCase)
{
	EXPECT_TRUE(IsStartableExtension(std::filesystem::path{ ".exe" }.native()));
	EXPECT_TRUE(IsStartableExtension(std::filesystem::path{ ".EXE" }.native()));
	EXPECT_TRUE(IsStartableExtension(std::filesystem::path{ ".Com" }.native()));
}

//////////////////////////////////////////////////////////////////////////
TEST(StartableExtension, BatchFilesAndOthersAreNot)
{
	EXPECT_FALSE(IsStartableExtension(std::filesystem::path{ ".bat" }.native()));
	EXPECT_FALSE(IsStartableExtension(std::filesystem::path{ ".cmd" }.native()));
	EXPECT_FALSE(IsStartableExtension(std::filesystem::path{ ".exe2" }.native()));
	EXPECT_FALSE(IsStartableExtension(std::filesystem::path{ "exe" }.native()));
	EXPECT_FALSE(IsStartableExtension(std::filesystem::path{}.native()));
}
