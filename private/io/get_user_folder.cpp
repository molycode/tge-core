#include <tge/io/get_user_folder.hpp>
#include <tge/platform.hpp>
#include <array>
#include <cstddef>

#if defined(TGE_PLATFORM_WINDOWS)
#include <string>
#include <string_view>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>
#elif defined(TGE_PLATFORM_LINUX)
#include "user_folder_error_category.hpp"
#include <cstdlib>
#include <string_view>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::IO
{
namespace
{
constexpr std::array<char const*, 4> OverrideVariables{ "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_STATE_HOME", "XDG_CACHE_HOME" };

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
std::filesystem::path GetEnvironmentPath(std::string_view name)
{
	std::wstring const wideName{ name.begin(), name.end() };
	DWORD const size{ GetEnvironmentVariableW(wideName.c_str(), nullptr, 0) };
	std::wstring value{};

	if (size > 0)
	{
		value.resize(size);

		DWORD const length{ GetEnvironmentVariableW(wideName.c_str(), value.data(), size) };

		value.resize((length < size) ? length : 0);
	}

	return std::filesystem::path{ value };
}

//////////////////////////////////////////////////////////////////////////
std::error_code ToErrorCode(HRESULT result)
{
	return (HRESULT_FACILITY(result) == FACILITY_WIN32) ? std::error_code{ HRESULT_CODE(result), std::system_category() }
		: std::error_code{ static_cast<int>(result), std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
// The roaming folder for what follows the user between machines, the local one for logs and caches.
std::expected<std::filesystem::path, std::error_code> GetKnownFolder(EUserFolder folder)
{
	KNOWNFOLDERID const& id{ (folder == EUserFolder::Config || folder == EUserFolder::Data) ? FOLDERID_RoamingAppData : FOLDERID_LocalAppData };
	PWSTR pPath{ nullptr };
	HRESULT const result{ SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &pPath) };
	std::expected<std::filesystem::path, std::error_code> known{ std::unexpected{ ToErrorCode(result) } };

	if (SUCCEEDED(result))
	{
		known = std::filesystem::path{ pPath };
	}

	CoTaskMemFree(pPath);

	return known;
}
#elif defined(TGE_PLATFORM_LINUX)
constexpr std::array<std::string_view, 4> HomeFolders{ ".config", ".local/share", ".local/state", ".cache" };

//////////////////////////////////////////////////////////////////////////
std::filesystem::path GetEnvironmentPath(char const* pName)
{
	char const* const pValue{ std::getenv(pName) };

	return (pValue != nullptr) ? std::filesystem::path{ pValue } : std::filesystem::path{};
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
// The XDG specification ignores a relative value, so a relative override falls back exactly like an unset one.
std::expected<std::filesystem::path, std::error_code> GetUserFolder(EUserFolder folder)
{
	size_t const index{ static_cast<size_t>(folder) };
	std::filesystem::path const overridden{ GetEnvironmentPath(OverrideVariables[index]) };
	std::expected<std::filesystem::path, std::error_code> result{ overridden };

	if (!overridden.is_absolute())
	{
#if defined(TGE_PLATFORM_WINDOWS)
		result = GetKnownFolder(folder);
#elif defined(TGE_PLATFORM_LINUX)
		std::filesystem::path const home{ GetEnvironmentPath("HOME") };

		result = home.is_absolute() ? std::expected<std::filesystem::path, std::error_code>{ home / HomeFolders[index] }
			: std::unexpected{ std::error_code{ NoHomeError, GetUserFolderErrorCategory() } };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
	}

	return result;
}
} // namespace Tge::IO
