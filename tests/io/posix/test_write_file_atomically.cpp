#include "write_file_atomically_test.hpp"
#include <gtest/gtest.h>
#include <tge/io/write_file_atomically.hpp>
#include <filesystem>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, KeepsThePermissions)
{
	std::error_code error{};

	WriteText(m_file, "first");
	std::filesystem::permissions(m_file, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, error);
	ASSERT_EQ(error, std::error_code{});

	ASSERT_TRUE(WriteFileAtomically(m_file, "second").has_value());
	EXPECT_EQ(std::filesystem::status(m_file, error).permissions(), std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, KeepsASymlinkedFileLinked)
{
	std::filesystem::path const target{ m_folder / "target.json" };
	std::error_code error{};

	WriteText(target, "first");
	std::filesystem::create_symlink(target, m_file, error);
	ASSERT_EQ(error, std::error_code{});

	ASSERT_TRUE(WriteFileAtomically(m_file, "second").has_value());
	EXPECT_TRUE(std::filesystem::is_symlink(m_file, error));
	EXPECT_EQ(ReadText(target), "second");
}
