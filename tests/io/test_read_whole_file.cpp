#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/io/read_whole_file.hpp>
#include <tge/platform.hpp>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

#if defined(TGE_PLATFORM_LINUX)
#include <sys/stat.h>
#endif // defined(TGE_PLATFORM_LINUX)

using namespace Tge::IO;
using namespace Tge::IO::Tests;

namespace
{
//////////////////////////////////////////////////////////////////////////
class CReadWholeFileTest : public testing::Test
{
protected:

	// testing::Test
	void SetUp() override
	{
		m_folder = MakeTemporaryFolder("tge-io-read");
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

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, DeviceIsRefused)
{
	std::expected<std::string, std::error_code> const read{ ReadWholeFile("NUL", 1024) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::invalid_argument);
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, FifoIsRefusedWithoutWaitingForAWriter)
{
	std::filesystem::path const fifo{ m_folder / "fifo" };

	ASSERT_EQ(::mkfifo(fifo.c_str(), 0600), 0);

	std::expected<std::string, std::error_code> const read{ ReadWholeFile(fifo, 1024) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::invalid_argument);
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
