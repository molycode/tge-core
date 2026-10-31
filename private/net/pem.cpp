#include "pem.hpp"
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace Tge::Net
{
namespace
{
constexpr std::string_view BeginMarker{ "-----BEGIN CERTIFICATE-----" };
constexpr std::string_view EndMarker{ "-----END CERTIFICATE-----" };
constexpr unsigned char DerSequenceTag{ 0x30 };
constexpr uint32_t NotBase64{ std::numeric_limits<uint32_t>::max() };

//////////////////////////////////////////////////////////////////////////
uint32_t ToSextet(char character)
{
	uint32_t sextet{ NotBase64 };

	if (character >= 'A' && character <= 'Z')
	{
		sextet = static_cast<uint32_t>(character - 'A');
	}
	else if (character >= 'a' && character <= 'z')
	{
		sextet = static_cast<uint32_t>(character - 'a') + 26;
	}
	else if (character >= '0' && character <= '9')
	{
		sextet = static_cast<uint32_t>(character - '0') + 52;
	}
	else if (character == '+')
	{
		sextet = 62;
	}
	else if (character == '/')
	{
		sextet = 63;
	}

	return sextet;
}

//////////////////////////////////////////////////////////////////////////
// White space between the characters is skipped; padding ends the data.
std::optional<std::vector<std::byte>> DecodeBase64(std::string_view text)
{
	std::vector<std::byte> bytes{};
	uint32_t accumulated{ 0 };
	uint32_t numBits{ 0 };
	bool hasPadding{ false };
	bool isValid{ true };

	for (size_t index{ 0 }; isValid && index < text.size(); ++index)
	{
		char const character{ text[index] };
		bool const isSpace{ character == ' ' || character == '\t' || character == '\r' || character == '\n' };

		if (character == '=')
		{
			hasPadding = true;
		}
		else if (!isSpace)
		{
			uint32_t const sextet{ ToSextet(character) };
			isValid = !hasPadding && sextet != NotBase64;

			if (isValid)
			{
				accumulated = ((accumulated << 6) | sextet) & 0xFFFF;
				numBits += 6;

				if (numBits >= 8)
				{
					numBits -= 8;
					bytes.push_back(static_cast<std::byte>((accumulated >> numBits) & 0xFF));
				}
			}
		}
	}

	return isValid ? std::optional<std::vector<std::byte>>{ std::move(bytes) } : std::nullopt;
}
} // namespace

//////////////////////////////////////////////////////////////////////////
// As Mbed TLS reads a certificate file: its PEM blocks, or, with no PEM header at all, the whole of it as one DER
// certificate if it starts as one, so a text file gives none. A block that is not base64 is skipped.
std::vector<std::vector<std::byte>> DecodeCertificates(std::string_view content)
{
	std::vector<std::vector<std::byte>> certificates{};
	size_t begin{ content.find(BeginMarker) };

	if (begin == std::string_view::npos && !content.empty() && static_cast<unsigned char>(content.front()) == DerSequenceTag)
	{
		std::byte const* const pContent{ reinterpret_cast<std::byte const*>(content.data()) };
		certificates.emplace_back(pContent, pContent + content.size());
	}

	while (begin != std::string_view::npos)
	{
		size_t const bodyStart{ begin + BeginMarker.size() };
		size_t const end{ content.find(EndMarker, bodyStart) };
		begin = std::string_view::npos;

		if (end != std::string_view::npos)
		{
			std::optional<std::vector<std::byte>> decoded{ DecodeBase64(content.substr(bodyStart, end - bodyStart)) };

			if (decoded.has_value() && !decoded->empty())
			{
				certificates.emplace_back(std::move(*decoded));
			}

			begin = content.find(BeginMarker, end + EndMarker.size());
		}
	}

	return certificates;
}
} // namespace Tge::Net
