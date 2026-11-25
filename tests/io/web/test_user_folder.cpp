#include "environment.hpp"
#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/io/write_file_atomically.hpp>
#include <array>
#include <filesystem>
#include <string>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST(UserFolder, EachHoldsItsOwnFile)
{
	constexpr std::array Folders{ EUserFolder::Config, EUserFolder::Data, EUserFolder::State, EUserFolder::Cache };

	for (EUserFolder const folder : Folders)
	{
		std::filesystem::path const path{ GetWith(folder, WithoutOverrides()).value_or(std::filesystem::path{}) };
		std::error_code error{};

		std::filesystem::create_directories(path, error);
		ASSERT_TRUE(path.is_absolute());
		ASSERT_TRUE(WriteFileAtomically(path / "file.txt", std::to_string(static_cast<int>(folder))).has_value());
	}

	for (EUserFolder const folder : Folders)
	{
		std::filesystem::path const path{ GetWith(folder, WithoutOverrides()).value_or(std::filesystem::path{}) };

		EXPECT_EQ(ReadText(path / "file.txt"), std::to_string(static_cast<int>(folder)));
	}
}
