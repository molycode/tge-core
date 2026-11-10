#pragma once

#include <format>
#include <string>
#include <string_view>
#include <cstdint>

namespace Tge::Assets
{
// Stable 128-bit identity for an asset, decoupled from its file path so it
// survives moves and renames. Generated once (RFC-4122 v4 random) on first
// import and persisted in a sidecar .meta file. The canonical text form is the
// standard 8-4-4-4-12 hexadecimal grouping.
struct SAssetGuid final
{
	uint64_t high{ 0 };
	uint64_t low{ 0 };
};

//////////////////////////////////////////////////////////////////////////
inline bool operator==(SAssetGuid const& a, SAssetGuid const& b)
{
	return a.high == b.high && a.low == b.low;
}

//////////////////////////////////////////////////////////////////////////
inline bool IsValid(SAssetGuid const& guid)
{
	return (guid.high | guid.low) != 0;
}

//////////////////////////////////////////////////////////////////////////
inline std::string ToString(SAssetGuid const& guid)
{
	return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
		static_cast<uint32_t>(guid.high >> 32),
		static_cast<uint32_t>((guid.high >> 16) & 0xFFFFu),
		static_cast<uint32_t>(guid.high & 0xFFFFu),
		static_cast<uint32_t>(guid.low >> 48),
		guid.low & 0x0000FFFFFFFFFFFFull);
}

//////////////////////////////////////////////////////////////////////////
inline bool FromString(std::string_view text, SAssetGuid& outGuid)
{
	// Accept the canonical form with or without separators by reading exactly
	// 32 hexadecimal nibbles and ignoring '-'. High and low each take 16.
	uint64_t high{ 0 };
	uint64_t low{ 0 };
	int numDigits{ 0 };
	bool valid{ true };

	for (char const c : text)
	{
		if (c != '-')
		{
			int nibble{ 0 };

			if (c >= '0' && c <= '9')
			{
				nibble = c - '0';
			}
			else if (c >= 'a' && c <= 'f')
			{
				nibble = c - 'a' + 10;
			}
			else if (c >= 'A' && c <= 'F')
			{
				nibble = c - 'A' + 10;
			}
			else
			{
				valid = false;
			}

			if (valid)
			{
				if (numDigits < 16)
				{
					high = (high << 4) | static_cast<uint64_t>(nibble);
				}
				else if (numDigits < 32)
				{
					low = (low << 4) | static_cast<uint64_t>(nibble);
				}
				else
				{
					valid = false;
				}

				++numDigits;
			}
		}
	}

	bool const success = valid && numDigits == 32;

	if (success)
	{
		outGuid.high = high;
		outGuid.low  = low;
	}

	return success;
}
} // namespace Tge::Assets

//////////////////////////////////////////////////////////////////////////
template <>
struct std::hash<Tge::Assets::SAssetGuid>
{
	size_t operator()(Tge::Assets::SAssetGuid const& guid) const noexcept
	{
		// Mix the two halves; 0x9e3779b97f4a7c15 is the 64-bit golden ratio.
		size_t seed = std::hash<uint64_t>{}(guid.high);
		seed ^= std::hash<uint64_t>{}(guid.low) + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
		return seed;
	}
};
