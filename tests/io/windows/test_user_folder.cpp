#include "environment.hpp"
#include <gtest/gtest.h>
#include <expected>
#include <filesystem>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, RoamingForConfigAndDataLocalForStateAndCache)
{
	std::expected<std::filesystem::path, std::error_code> const config{ GetWith(EUserFolder::Config, WithoutOverrides()) };
	std::expected<std::filesystem::path, std::error_code> const data{ GetWith(EUserFolder::Data, WithoutOverrides()) };
	std::expected<std::filesystem::path, std::error_code> const state{ GetWith(EUserFolder::State, WithoutOverrides()) };
	std::expected<std::filesystem::path, std::error_code> const cache{ GetWith(EUserFolder::Cache, WithoutOverrides()) };
	std::error_code error{};

	ASSERT_TRUE(config.has_value() && data.has_value() && state.has_value() && cache.has_value());
	EXPECT_EQ(*config, *data);
	EXPECT_EQ(*state, *cache);
	EXPECT_NE(*config, *state);
	EXPECT_TRUE(std::filesystem::is_directory(*config, error));
	EXPECT_TRUE(std::filesystem::is_directory(*state, error));
}
