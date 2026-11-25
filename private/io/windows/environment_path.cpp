#include "io/environment_path.hpp"
#include <string>
#include <string_view>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
std::filesystem::path GetEnvironmentPath(char const* pName)
{
	std::string_view const name{ pName };
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
} // namespace Tge::IO
