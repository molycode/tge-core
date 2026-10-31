#include <tge/net/parse_ipv4.hpp>
#include <array>

namespace Tge::Net
{
//////////////////////////////////////////////////////////////////////////
// inet_pton's rules: four dotted decimal parts of 0 to 255, nothing else, and no part with a leading zero.
std::optional<uint32_t> ParseIpv4(std::string_view text)
{
	constexpr uint32_t NumParts{ 4 };
	std::array<uint32_t, NumParts> parts{};
	uint32_t numParts{ 0 };
	bool hasDigit{ false };
	bool isValid{ true };

	for (size_t index{ 0 }; isValid && index < text.size(); ++index)
	{
		char const character{ text[index] };

		if (character >= '0' && character <= '9')
		{
			bool const startsPart{ !hasDigit };
			isValid = startsPart ? numParts < NumParts : parts[numParts - 1] != 0;

			if (isValid)
			{
				numParts += startsPart ? 1u : 0u;
				uint32_t const value{ parts[numParts - 1] * 10 + static_cast<uint32_t>(character - '0') };
				isValid = value <= 255;
				parts[numParts - 1] = value;
				hasDigit = true;
			}
		}
		else
		{
			isValid = character == '.' && hasDigit && numParts < NumParts;
			hasDigit = false;
		}
	}

	std::optional<uint32_t> address{};

	if (isValid && numParts == NumParts && hasDigit)
	{
		address = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
	}

	return address;
}
} // namespace Tge::Net
