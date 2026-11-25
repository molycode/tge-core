#include "write_file_atomically_test.hpp"
#include <gtest/gtest.h>
#include <tge/io/write_file_atomically.hpp>
#include <filesystem>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, ReplacesTheFileInsteadOfRewritingIt)
{
	std::error_code error{};

	WriteText(m_file, "first");
	std::filesystem::create_hard_link(m_file, m_folder / "kept", error);
	ASSERT_EQ(error, std::error_code{});

	ASSERT_TRUE(WriteFileAtomically(m_file, "second").has_value());
	EXPECT_EQ(ReadText(m_file), "second");
	EXPECT_EQ(ReadText(m_folder / "kept"), "first");
}
