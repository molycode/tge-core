#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

namespace Tge::Command
{
// Under -fno-exceptions std::stof/std::stoi terminate on bad input, so a console typo would kill the process.
inline bool ParseFloat(std::string_view const text, float& outValue)
{
	char const* const first = text.data();
	char const* const last  = text.data() + text.size();

	std::from_chars_result const result = std::from_chars(first, last, outValue);

	return result.ec == std::errc{} && result.ptr == last;
}

inline bool ParseIndex(std::string_view const text, uint32_t& outValue)
{
	char const* const first = text.data();
	char const* const last  = text.data() + text.size();

	std::from_chars_result const result = std::from_chars(first, last, outValue);

	return result.ec == std::errc{} && result.ptr == last;
}
} // namespace Tge::Command
