#include "startable_extension.hpp"
#include <algorithm>
#include <array>

namespace Tge::Process
{
namespace
{
constexpr std::array<std::string_view, 2> StartableExtensions{ ".exe", ".com" };
} // namespace

//////////////////////////////////////////////////////////////////////////
// Only what Windows starts by itself: a batch file runs through cmd.exe, which would parse the arguments again.
bool IsStartableExtension(std::basic_string_view<std::filesystem::path::value_type> extension)
{
	auto const isSameLetter{ [](std::filesystem::path::value_type actual, char expected)
	{
		bool const isUpper{ actual >= 'A' && actual <= 'Z' };

		return (isUpper ? actual - 'A' + 'a' : actual) == expected;
	} };

	return std::ranges::any_of(StartableExtensions, [&](std::string_view startable)
	{
		return std::ranges::equal(extension, startable, isSameLetter);
	});
}
} // namespace Tge::Process
