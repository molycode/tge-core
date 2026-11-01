#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/process/is_runnable.hpp>
#include <filesystem>
#include <system_error>

using namespace Tge::Process;
using namespace Tge::Process::Tests;

//////////////////////////////////////////////////////////////////////////
TEST(IsRunnable, AProgram)
{
	EXPECT_TRUE(IsRunnable(GetChildProgram()));
}

//////////////////////////////////////////////////////////////////////////
TEST(IsRunnable, NotATextFileAFolderOrNothing)
{
	std::filesystem::path const folder{ MakeTemporaryFolder("tge-process-runnable") };
	std::error_code error{};

	ASSERT_FALSE(folder.empty());
	WriteText(folder / "notes.txt", "notes");

	EXPECT_FALSE(IsRunnable(folder / "notes.txt"));
	EXPECT_FALSE(IsRunnable(folder));
	EXPECT_FALSE(IsRunnable(folder / "missing"));

	std::filesystem::remove_all(folder, error);
}
