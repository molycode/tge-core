#include "io/default_user_folder.hpp"
#include "io/web/user_folders.hpp"
#include <array>
#include <cstddef>
#include <cstdlib>
#include <string_view>

namespace Tge::IO
{
namespace
{
constexpr std::array<std::string_view, 4> Folders{ "config", "data", "state", "cache" };
} // namespace

//////////////////////////////////////////////////////////////////////////
std::expected<std::filesystem::path, std::error_code> GetDefaultUserFolder(EUserFolder folder)
{
	char* const pRoot{ TgeGetUserFoldersRoot() };
	std::filesystem::path const path{ std::filesystem::path{ pRoot } / Folders[static_cast<size_t>(folder)] };

	std::free(pRoot);

	return path;
}
} // namespace Tge::IO
