#include "io/default_user_folder.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>

namespace Tge::IO
{
namespace
{
//////////////////////////////////////////////////////////////////////////
std::error_code ToErrorCode(HRESULT result)
{
	return (HRESULT_FACILITY(result) == FACILITY_WIN32) ? std::error_code{ HRESULT_CODE(result), std::system_category() }
		: std::error_code{ static_cast<int>(result), std::system_category() };
}
} // namespace

//////////////////////////////////////////////////////////////////////////
// The roaming folder for what follows the user between machines, the local one for logs and caches.
std::expected<std::filesystem::path, std::error_code> GetDefaultUserFolder(EUserFolder folder)
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
} // namespace Tge::IO
