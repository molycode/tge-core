#include "test_support.hpp"
#include <gtest/gtest.h>
#include <tge/io/is_busy.hpp>
#include <tge/platform.hpp>
#include <filesystem>
#include <fstream>
#include <system_error>

using namespace Tge::IO;
using namespace Tge::IO::Tests;

//////////////////////////////////////////////////////////////////////////
// What a virus scanner does to a folder just written: it holds a file inside open while it reads it.
TEST(IsBusy, FolderWithAFileOpenInside)
{
	std::filesystem::path const folder{ MakeTemporaryFolder("tge-io-busy") };
	std::error_code error{};

	ASSERT_FALSE(folder.empty());
	std::filesystem::create_directory(folder / "game", error);
	WriteText(folder / "game" / "game.json", "{}");

	std::ifstream held{ folder / "game" / "game.json" };

	ASSERT_TRUE(held.is_open());
	std::filesystem::rename(folder / "game", folder / "renamed", error);
	held.close();

#if defined(TGE_PLATFORM_WINDOWS)
	EXPECT_TRUE(IsBusy(error)) << error.message();
#elif defined(TGE_PLATFORM_LINUX)
	EXPECT_EQ(error, std::error_code{});
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	std::filesystem::remove_all(folder, error);
}

//////////////////////////////////////////////////////////////////////////
TEST(IsBusy, MissingFileIsNotBusy)
{
	std::error_code error{};

	std::filesystem::rename("tge-io-tests-missing", "tge-io-tests-renamed", error);

	ASSERT_NE(error, std::error_code{});
	EXPECT_FALSE(IsBusy(error));
}
