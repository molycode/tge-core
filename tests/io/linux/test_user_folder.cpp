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
TEST(UserFolder, HomeIsTheFallback)
{
	Variables variables{ WithoutOverrides() };

	variables.emplace_back("HOME", std::string{ "/home/tester" });

	EXPECT_EQ(GetWith(EUserFolder::Config, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.config" });
	EXPECT_EQ(GetWith(EUserFolder::Data, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.local/share" });
	EXPECT_EQ(GetWith(EUserFolder::State, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.local/state" });
	EXPECT_EQ(GetWith(EUserFolder::Cache, variables).value_or(std::filesystem::path{}), std::filesystem::path{ "/home/tester/.cache" });
}

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, NoHomeIsAnError)
{
	std::expected<std::filesystem::path, std::error_code> const folder{ GetWith(EUserFolder::State, { { "XDG_STATE_HOME", std::nullopt }, { "HOME", std::nullopt } }) };

	ASSERT_FALSE(folder.has_value());
	EXPECT_EQ(folder.error().message(), "HOME is unset or not an absolute path");
}
