#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/io/file.hpp>
#include <filesystem>
#include <string>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

namespace
{
//////////////////////////////////////////////////////////////////////////
class CFileTest : public testing::Test
{
protected:

	// testing::Test
	void SetUp() override
	{
		m_folder = MakeTemporaryFolder("tge-io-file");
		ASSERT_FALSE(m_folder.empty());
	}

	void TearDown() override
	{
		std::error_code error{};

		std::filesystem::remove_all(m_folder, error);
	}
	// ~testing::Test

	std::filesystem::path m_folder{};
};
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST_F(CFileTest, ReadsTextWithCrlfLineEndings)
{
	WriteText(m_folder / "crlf.txt", "first\r\nsecond\r\n");

	std::string text{};

	ASSERT_TRUE(CFile::ReadAllText((m_folder / "crlf.txt").string(), text));

	std::erase(text, '\r');
	EXPECT_EQ(text, "first\nsecond\n");
}
