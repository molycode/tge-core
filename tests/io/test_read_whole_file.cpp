#include "read_whole_file_test.hpp"
#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/io/read_whole_file.hpp>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, ReadsEveryByte)
{
	std::string const content{ std::string_view{ "line\r\nnul \0 byte\n", 17 } };

	WriteText(m_folder / "file.bin", content);

	std::expected<std::string, std::error_code> const read{ ReadWholeFile(m_folder / "file.bin", 1024) };

	ASSERT_TRUE(read.has_value()) << read.error().message();
	EXPECT_EQ(*read, content);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, ReadsAnEmptyFile)
{
	WriteText(m_folder / "empty.txt", "");

	std::expected<std::string, std::error_code> const read{ ReadWholeFile(m_folder / "empty.txt", 1024) };

	ASSERT_TRUE(read.has_value()) << read.error().message();
	EXPECT_TRUE(read->empty());
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, MissingFileIsNoSuchFile)
{
	std::expected<std::string, std::error_code> const read{ ReadWholeFile(m_folder / "missing.txt", 1024) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::no_such_file_or_directory);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, MissingFolderIsNoSuchFile)
{
	std::expected<std::string, std::error_code> const read{ ReadWholeFile(m_folder / "missing" / "file.txt", 1024) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::no_such_file_or_directory);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, FolderIsRefused)
{
	std::expected<std::string, std::error_code> const read{ ReadWholeFile(m_folder, 1024) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::is_a_directory);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, LargerThanTheCapIsRefusedBeforeReading)
{
	WriteText(m_folder / "big.txt", "0123456789A");

	std::expected<std::string, std::error_code> const read{ ReadWholeFile(m_folder / "big.txt", 10) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::file_too_large);
}
