#include "io/resolve_link.hpp"
#include <system_error>

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
// A link a dotfile manager put in the file's place survives, since the rename replaces its target instead.
std::filesystem::path ResolveLink(std::filesystem::path const& path)
{
	std::error_code error{};
	std::filesystem::path const target{ std::filesystem::canonical(path, error) };

	return (error == std::error_code{}) ? target : path;
}
} // namespace Tge::IO
