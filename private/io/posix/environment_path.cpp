#include "io/environment_path.hpp"
#include <cstdlib>

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
std::filesystem::path GetEnvironmentPath(char const* pName)
{
	char const* const pValue{ std::getenv(pName) };

	return (pValue != nullptr) ? std::filesystem::path{ pValue } : std::filesystem::path{};
}
} // namespace Tge::IO
