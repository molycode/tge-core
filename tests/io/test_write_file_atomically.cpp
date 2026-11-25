#include "write_file_atomically_test.hpp"
#include <gtest/gtest.h>
#include <tge/io/write_file_atomically.hpp>
#include <expected>
#include <filesystem>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, CreatesAMissingFile)
{
	ASSERT_TRUE(WriteFileAtomically(m_file, "first").has_value());
	EXPECT_EQ(ReadText(m_file), "first");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, LeavesNoTemporaryFile)
{
	ASSERT_TRUE(WriteFileAtomically(m_file, "first").has_value());
	ASSERT_TRUE(WriteFileAtomically(m_file, "second").has_value());
	EXPECT_EQ(CountEntries(m_folder), 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, FolderInTheWayFailsAndLeavesNothing)
{
	std::error_code error{};

	std::filesystem::create_directory(m_file, error);
	ASSERT_EQ(error, std::error_code{});

	EXPECT_FALSE(WriteFileAtomically(m_file, "text").has_value());
	EXPECT_EQ(CountEntries(m_folder), 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, MissingFolderIsNoSuchFile)
{
	std::expected<void, std::error_code> const written{ WriteFileAtomically(m_folder / "missing" / "file.txt", "text") };

	ASSERT_FALSE(written.has_value());
	EXPECT_EQ(written.error(), std::errc::no_such_file_or_directory);
}
