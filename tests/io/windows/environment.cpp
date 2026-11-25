#include "environment.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

namespace Tge::IO::Tests
{
//////////////////////////////////////////////////////////////////////////
std::optional<std::string> GetVariable(char const* pName)
{
	std::optional<std::string> value{};
	std::wstring const name{ std::filesystem::path{ pName }.wstring() };
	DWORD const size{ GetEnvironmentVariableW(name.c_str(), nullptr, 0) };

	if (size > 0)
	{
		std::wstring wide{};

		wide.resize(size);
		wide.resize(GetEnvironmentVariableW(name.c_str(), wide.data(), size));
		value = std::filesystem::path{ wide }.string();
	}

	return value;
}

//////////////////////////////////////////////////////////////////////////
void SetVariable(char const* pName, std::optional<std::string> const& value)
{
	std::wstring const name{ std::filesystem::path{ pName }.wstring() };
	std::wstring const wide{ value.has_value() ? std::filesystem::path{ *value }.wstring() : std::wstring{} };

	SetEnvironmentVariableW(name.c_str(), value.has_value() ? wide.c_str() : nullptr);
}
} // namespace Tge::IO::Tests
