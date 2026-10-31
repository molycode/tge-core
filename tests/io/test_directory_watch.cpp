#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/io/directory_watch.hpp>
#include <tge/platform.hpp>
#include <tge/threading/event_loop.hpp>
#include <algorithm>
#include <condition_variable>
#include <expected>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <semaphore>
#include <system_error>
#include <vector>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

namespace
{
//////////////////////////////////////////////////////////////////////////
// The watch runs on a loop, as its owners run it; what it reports is gathered across callbacks, since one read may
// carry only part of a burst.
class CDirectoryWatchTest : public testing::Test
{
protected:

	// testing::Test
	void SetUp() override
	{
		m_folder = MakeTemporaryFolder("tge-io-watch");
		ASSERT_FALSE(m_folder.empty());
		ASSERT_TRUE(m_loop.Initialize("TgeTestWatch"));
	}

	void TearDown() override
	{
		std::error_code error{};

		Unwatch();
		m_loop.Terminate();
		m_watch.Terminate();
		std::filesystem::remove_all(m_folder, error);
	}
	// ~testing::Test

	void Watch(std::filesystem::path const& folder, EWatchScope scope)
	{
		std::binary_semaphore watched{ 0 };

		ASSERT_TRUE(m_watch.Initialize(folder, scope).has_value());

		m_loop.Post([this, &watched]()
		{
			m_watchId = m_loop.Watch(m_watch.GetWaitable(), [this]() { Collect(); });
			watched.release();
		});

		ASSERT_TRUE(watched.try_acquire_for(Patience));
		ASSERT_TRUE(m_watchId.has_value());
	}

	void Unwatch()
	{
		std::binary_semaphore unwatched{ 0 };

		m_loop.Post([this, &unwatched]()
		{
			if (m_watchId.has_value())
			{
				m_loop.Unwatch(*m_watchId);
				m_watchId.reset();
			}

			unwatched.release();
		});

		EXPECT_TRUE(unwatched.try_acquire_for(Patience));
	}

	bool WaitFor(std::function<bool()> const& isDone)
	{
		std::unique_lock lock{ m_mutex };

		return m_changed.wait_for(lock, Patience, isDone);
	}

	bool HasPath(std::filesystem::path const& path) const
	{
		return std::ranges::find(m_paths, path) != m_paths.end();
	}

	std::filesystem::path m_folder{};
	Tge::Threading::CEventLoop m_loop{};
	CDirectoryWatch m_watch{};
	std::optional<Tge::Threading::SWatchId> m_watchId{};
	std::mutex m_mutex{};
	std::condition_variable m_changed{};
	std::vector<std::filesystem::path> m_paths{};
	std::optional<std::error_code> m_ended{};

private:

	void Collect()
	{
		std::expected<SDirectoryChanges, std::error_code> changes{ m_watch.ReadChanges() };
		std::lock_guard const lock{ m_mutex };

		if (changes.has_value())
		{
			m_paths.insert(m_paths.end(), changes->paths.begin(), changes->paths.end());
		}
		else
		{
			m_ended = changes.error();
			m_loop.Unwatch(*m_watchId);
			m_watchId.reset();
		}

		m_changed.notify_all();
	}
};
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, TreeReportsANestedWriteRelativeToTheFolder)
{
	std::error_code error{};

	std::filesystem::create_directories(m_folder / "games" / "quake2", error);
	Watch(m_folder, EWatchScope::Tree);
	WriteText(m_folder / "games" / "quake2" / "game.json", "{}");

	EXPECT_TRUE(WaitFor([this]() { return HasPath(std::filesystem::path{ "games" } / "quake2" / "game.json"); }));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, TreeReportsAFolderMadeLaterAndWhatIsWrittenInIt)
{
	std::error_code error{};

	Watch(m_folder, EWatchScope::Tree);
	std::filesystem::create_directory(m_folder / "games", error);
	ASSERT_TRUE(WaitFor([this]() { return HasPath("games"); }));

	WriteText(m_folder / "games" / "game.json", "{}");

	EXPECT_TRUE(WaitFor([this]() { return HasPath(std::filesystem::path{ "games" } / "game.json"); }));
}

#if defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, TreeFollowsASymlinkedFolderMadeLater)
{
	std::filesystem::path const target{ m_folder / "target" };
	std::filesystem::path const watched{ m_folder / "watched" };
	std::error_code error{};

	std::filesystem::create_directory(target, error);
	std::filesystem::create_directory(watched, error);
	Watch(watched, EWatchScope::Tree);
	std::filesystem::create_directory_symlink(target, watched / "linked", error);
	ASSERT_EQ(error, std::error_code{});
	ASSERT_TRUE(WaitFor([this]() { return HasPath("linked"); }));

	WriteText(target / "game.json", "{}");

	EXPECT_TRUE(WaitFor([this]() { return HasPath(std::filesystem::path{ "linked" } / "game.json"); }));
}
#endif // defined(TGE_PLATFORM_LINUX)

//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, FolderScopeLeavesOutWhatIsBelowItsFolders)
{
	std::error_code error{};

	std::filesystem::create_directory(m_folder / "games", error);
	Watch(m_folder, EWatchScope::Folder);
	WriteText(m_folder / "games" / "game.json", "{}");
	WriteText(m_folder / "top.txt", "text");

	ASSERT_TRUE(WaitFor([this]() { return HasPath("top.txt"); }));

	std::lock_guard const lock{ m_mutex };

	EXPECT_FALSE(HasPath(std::filesystem::path{ "games" } / "game.json"));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, RemovingTheFolderEndsTheWatch)
{
	std::filesystem::path const watched{ m_folder / "watched" };
	std::error_code error{};

	std::filesystem::create_directory(watched, error);
	Watch(watched, EWatchScope::Tree);
	std::filesystem::remove_all(watched, error);

	EXPECT_TRUE(WaitFor([this]() { return m_ended.has_value(); }));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, MissingFolderCannotBeWatched)
{
	std::expected<void, std::error_code> const started{ m_watch.Initialize(m_folder / "missing", EWatchScope::Tree) };

	ASSERT_FALSE(started.has_value());
	EXPECT_EQ(started.error(), std::errc::no_such_file_or_directory);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, FileCannotBeWatched)
{
	WriteText(m_folder / "file.txt", "text");

	std::expected<void, std::error_code> const started{ m_watch.Initialize(m_folder / "file.txt", EWatchScope::Tree) };

	ASSERT_FALSE(started.has_value());
	EXPECT_EQ(started.error(), std::errc::not_a_directory);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CDirectoryWatchTest, TerminatesWithAReadPending)
{
	Watch(m_folder, EWatchScope::Tree);
	Unwatch();
	m_watch.Terminate();

	SUCCEED();
}
