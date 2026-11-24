#include "read_whole_file_test.hpp"
#include <gtest/gtest.h>
#include <tge/io/read_whole_file.hpp>
#include <expected>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, FifoIsRefusedWithoutWaitingForAWriter)
{
	std::filesystem::path const fifo{ m_folder / "fifo" };

	ASSERT_EQ(::mkfifo(fifo.c_str(), 0600), 0);

	std::expected<std::string, std::error_code> const read{ ReadWholeFile(fifo, 1024) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::invalid_argument);
}
