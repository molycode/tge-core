#include "write_file_atomically_test.hpp"
#include <gtest/gtest.h>
#include <tge/io/is_busy.hpp>
#include <tge/io/write_file_atomically.hpp>
#include <expected>
#include <filesystem>
#include <system_error>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
// As a virus scanner or an indexer holds a file.
TEST_F(CWriteFileAtomicallyTest, ReplacesAFileHeldOpenWithDeleteSharing)
{
	WriteText(m_file, "first");

	HANDLE const held{ CreateFileW(m_file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr) };

	ASSERT_NE(held, INVALID_HANDLE_VALUE);

	std::expected<void, std::error_code> const written{ WriteFileAtomically(m_file, "second") };

	CloseHandle(held);
	ASSERT_TRUE(written.has_value()) << written.error().message();
	EXPECT_EQ(ReadText(m_file), "second");
	EXPECT_EQ(CountEntries(m_folder), 1u);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, FileHeldWithoutDeleteSharingIsBusy)
{
	WriteText(m_file, "first");

	HANDLE const held{ CreateFileW(m_file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr) };

	ASSERT_NE(held, INVALID_HANDLE_VALUE);

	std::expected<void, std::error_code> const written{ WriteFileAtomically(m_file, "second") };

	CloseHandle(held);
	ASSERT_FALSE(written.has_value());
	EXPECT_TRUE(IsBusy(written.error())) << written.error().message();
	EXPECT_EQ(ReadText(m_file), "first");
	EXPECT_EQ(CountEntries(m_folder), 1u);
}
