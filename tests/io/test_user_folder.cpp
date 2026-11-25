#include "environment.hpp"
#include <gtest/gtest.h>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, AbsoluteOverrideWins)
{
	std::error_code error{};
	std::filesystem::path const overridden{ std::filesystem::temp_directory_path(error) / "tge-io-tests-cache" };

	ASSERT_EQ(error, std::error_code{});

	std::expected<std::filesystem::path, std::error_code> const folder{ GetWith(EUserFolder::Cache, { { "XDG_CACHE_HOME", overridden.string() } }) };

	ASSERT_TRUE(folder.has_value());
	EXPECT_EQ(*folder, overridden);
}

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, RelativeOverrideIsIgnored)
{
	std::expected<std::filesystem::path, std::error_code> const relative{ GetWith(EUserFolder::Data,
		{ { "XDG_DATA_HOME", std::string{ "relative/data" } }, { "HOME", std::string{ "/home/tester" } } }) };
	std::expected<std::filesystem::path, std::error_code> const unset{ GetWith(EUserFolder::Data,
		{ { "XDG_DATA_HOME", std::nullopt }, { "HOME", std::string{ "/home/tester" } } }) };

	ASSERT_TRUE(relative.has_value());
	ASSERT_TRUE(unset.has_value());
	EXPECT_EQ(*relative, *unset);
	EXPECT_TRUE(relative->is_absolute());
}
