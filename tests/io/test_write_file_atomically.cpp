#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/io/is_busy.hpp>
#include <tge/io/read_whole_file.hpp>
#include <tge/io/write_file_atomically.hpp>
#include <tge/platform.hpp>
#include <expected>
#include <filesystem>
#include <string>
#include <system_error>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#endif // defined(TGE_PLATFORM_WINDOWS)

using namespace Tge::IO;
using namespace Tge::IO::Tests;

namespace
{
//////////////////////////////////////////////////////////////////////////
class CWriteFileAtomicallyTest : public testing::Test
{
protected:

	// testing::Test
	void SetUp() override
	{
		m_folder = MakeTemporaryFolder("tge-io-write");
		ASSERT_FALSE(m_folder.empty());
		m_file = m_folder / "settings.json";
	}

	void TearDown() override
	{
		std::error_code error{};

		std::filesystem::remove_all(m_folder, error);
	}
	// ~testing::Test

	std::filesystem::path m_folder{};
	std::filesystem::path m_file{};
};
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST_F(CWriteFileAtomicallyTest, CreatesAMissingFile)
{
	ASSERT_TRUE(WriteFileAtomically(m_file, "first").has_value());
	EXPECT_EQ(ReadText(m_file), "first");
}

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

#if defined(TGE_PLATFORM_WINDOWS)
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
#elif defined(TGE_PLATFORM_LINUX)
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
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
