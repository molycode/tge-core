#include <gtest/gtest.h>
#include <tge/process/get_program_path.hpp>
#include <expected>
#include <filesystem>
#include <system_error>

using namespace Tge::Process;

//////////////////////////////////////////////////////////////////////////
TEST(GetProgramPath, IsThisSuite)
{
	std::expected<std::filesystem::path, std::error_code> const path{ GetProgramPath() };
	std::error_code error{};

	ASSERT_TRUE(path.has_value()) << path.error().message();
	EXPECT_TRUE(path->is_absolute());
	EXPECT_TRUE(std::filesystem::equivalent(*path, TGE_PROCESS_TESTS, error)) << path->string();
}
