#include <tge/io/get_user_folder.hpp>
#include "default_user_folder.hpp"
#include "environment_path.hpp"
#include <array>
#include <cstddef>

namespace Tge::IO
{
namespace
{
constexpr std::array<char const*, 4> OverrideVariables{ "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_STATE_HOME", "XDG_CACHE_HOME" };
} // namespace

//////////////////////////////////////////////////////////////////////////
// The XDG specification ignores a relative value, so a relative override falls back exactly like an unset one.
std::expected<std::filesystem::path, std::error_code> GetUserFolder(EUserFolder folder)
{
	std::filesystem::path const overridden{ GetEnvironmentPath(OverrideVariables[static_cast<size_t>(folder)]) };
	std::expected<std::filesystem::path, std::error_code> result{ overridden };

	if (!overridden.is_absolute())
	{
		result = GetDefaultUserFolder(folder);
	}

	return result;
}
} // namespace Tge::IO
