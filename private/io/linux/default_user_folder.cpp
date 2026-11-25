#include "io/default_user_folder.hpp"
#include "io/environment_path.hpp"
#include "io/linux/user_folder_error_category.hpp"
#include <array>
#include <cstddef>
#include <string_view>

namespace Tge::IO
{
namespace
{
constexpr std::array<std::string_view, 4> HomeFolders{ ".config", ".local/share", ".local/state", ".cache" };
} // namespace

//////////////////////////////////////////////////////////////////////////
std::expected<std::filesystem::path, std::error_code> GetDefaultUserFolder(EUserFolder folder)
{
	std::filesystem::path const home{ GetEnvironmentPath("HOME") };

	return home.is_absolute() ? std::expected<std::filesystem::path, std::error_code>{ home / HomeFolders[static_cast<size_t>(folder)] }
		: std::unexpected{ std::error_code{ NoHomeError, GetUserFolderErrorCategory() } };
}
} // namespace Tge::IO
