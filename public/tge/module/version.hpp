#pragma once

#include <cstdint>
#include <format>

namespace Tge
{
struct SVersion final
{
	uint16_t major{ 0 };
	uint16_t minor{ 0 };
	uint16_t patch{ 0 };
};

// A major bump is a breaking interface change, so a NEWER major fails the same as a too-old one.
constexpr bool Satisfies(SVersion const& have, SVersion const& required)
{
	return have.major == required.major
	    && (have.minor > required.minor
	     || (have.minor == required.minor && have.patch >= required.patch));
}

// A dependency need not state a version at all; an unset constraint is satisfied by anything.
constexpr bool IsConstrained(SVersion const& version)
{
	return version.major != 0 || version.minor != 0 || version.patch != 0;
}
} // namespace Tge

template<>
struct std::formatter<Tge::SVersion>
{
	constexpr auto parse(std::format_parse_context& context) const { return context.begin(); }

	auto format(Tge::SVersion const& version, std::format_context& context) const
	{
		return std::format_to(context.out(), "{}.{}.{}", version.major, version.minor, version.patch);
	}
};
