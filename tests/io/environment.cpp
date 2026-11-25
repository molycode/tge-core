#include "environment.hpp"
#include <tge/io/get_user_folder.hpp>

namespace Tge::IO::Tests
{
//////////////////////////////////////////////////////////////////////////
std::expected<std::filesystem::path, std::error_code> GetWith(EUserFolder folder, Variables const& variables)
{
	Variables previous{};

	for (auto const& [pName, value] : variables)
	{
		previous.emplace_back(pName, GetVariable(pName));
		SetVariable(pName, value);
	}

	std::expected<std::filesystem::path, std::error_code> const result{ GetUserFolder(folder) };

	for (auto const& [pName, value] : previous)
	{
		SetVariable(pName, value);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
Variables WithoutOverrides()
{
	return Variables{ { "XDG_CONFIG_HOME", std::nullopt }, { "XDG_DATA_HOME", std::nullopt }, { "XDG_STATE_HOME", std::nullopt },
		{ "XDG_CACHE_HOME", std::nullopt } };
}
} // namespace Tge::IO::Tests
