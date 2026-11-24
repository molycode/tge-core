#include "read_whole_file_test.hpp"
#include <gtest/gtest.h>
#include <tge/io/read_whole_file.hpp>
#include <expected>
#include <string>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
TEST_F(CReadWholeFileTest, DeviceIsRefused)
{
	std::expected<std::string, std::error_code> const read{ ReadWholeFile("NUL", 1024) };

	ASSERT_FALSE(read.has_value());
	EXPECT_EQ(read.error(), std::errc::invalid_argument);
}
