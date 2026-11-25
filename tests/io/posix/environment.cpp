#include "environment.hpp"
#include <cstdlib>

namespace Tge::IO::Tests
{
//////////////////////////////////////////////////////////////////////////
std::optional<std::string> GetVariable(char const* pName)
{
	std::optional<std::string> value{};
	char const* const pValue{ std::getenv(pName) };

	if (pValue != nullptr)
	{
		value = std::string{ pValue };
	}

	return value;
}

//////////////////////////////////////////////////////////////////////////
void SetVariable(char const* pName, std::optional<std::string> const& value)
{
	if (value.has_value())
	{
		setenv(pName, value->c_str(), 1);
	}
	else
	{
		unsetenv(pName);
	}
}
} // namespace Tge::IO::Tests
