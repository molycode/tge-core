#include "pem.hpp"
#include <gtest/gtest.h>
#include <tge/net/verify_server_chain.hpp>
#include <tge/platform.hpp>
#include <cstddef>
#include <expected>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(TGE_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#endif // defined(TGE_PLATFORM_WINDOWS)

using namespace Tge::Net;

namespace
{
// Self-signed for tge-core.test, valid until 2126, for server authentication; no system trusts it.
constexpr std::string_view UnknownRoot{
	"-----BEGIN CERTIFICATE-----\n"
	"MIIBwzCCAWigAwIBAgIUcXT6y3KTf8+imCAVuCyYFHnMK4QwCgYIKoZIzj0EAwIw\n"
	"HTEbMBkGA1UEAwwSdGdlLWNvcmUgdGVzdCByb290MCAXDTI2MTEwMjEwMzQzOVoY\n"
	"DzIxMjYxMDA5MTAzNDM5WjAdMRswGQYDVQQDDBJ0Z2UtY29yZSB0ZXN0IHJvb3Qw\n"
	"WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASsfdJzqzg2mQryoJwoFzJm2cv9x9LB\n"
	"+fKEBIeI2p56zo6MIP3o95VluQKISzLRPwBhvQUUhFf0mEmBWr6sDykHo4GDMIGA\n"
	"MB0GA1UdDgQWBBTY2cHtgVhXxOjJzEBDuEpghkSCDDAfBgNVHSMEGDAWgBTY2cHt\n"
	"gVhXxOjJzEBDuEpghkSCDDAPBgNVHRMBAf8EBTADAQH/MBMGA1UdJQQMMAoGCCsG\n"
	"AQUFBwMBMBgGA1UdEQQRMA+CDXRnZS1jb3JlLnRlc3QwCgYIKoZIzj0EAwIDSQAw\n"
	"RgIhALlFeAEOgkL07EFy+sKY54Hla9CGq0GdWrfLbLysWP7pAiEAtwisKwt3I9Rh\n"
	"voQSk4qA6EPZnEAsPPQt6dXB0Fiyrfw=\n"
	"-----END CERTIFICATE-----\n" };

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> VerifyAlone(std::vector<std::byte> const& certificate)
{
	std::span<std::byte const> const only{ certificate };

	return VerifyServerChain(std::span{ &only, 1 }, "tge-core.test");
}
} // namespace

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
TEST(VerifyServerChain, RefusesARootNoSystemTrusts)
{
	std::vector<std::vector<std::byte>> const certificates{ DecodeCertificates(UnknownRoot) };

	ASSERT_EQ(certificates.size(), 1u);

	std::expected<void, std::error_code> const verified{ VerifyAlone(certificates.front()) };

	ASSERT_FALSE(verified.has_value());
	EXPECT_EQ(verified.error().value(), static_cast<int>(CERT_E_UNTRUSTEDROOT)) << verified.error().message();
}

//////////////////////////////////////////////////////////////////////////
TEST(VerifyServerChain, RefusesBytesThatAreNoCertificate)
{
	std::vector<std::byte> const notACertificate{ std::byte{ 0x30 }, std::byte{ 0x03 }, std::byte{ 0x02 }, std::byte{ 0x01 }, std::byte{ 0x05 } };

	EXPECT_FALSE(VerifyAlone(notACertificate).has_value());
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
TEST(VerifyServerChain, IsNotSupported)
{
	std::vector<std::vector<std::byte>> const certificates{ DecodeCertificates(UnknownRoot) };

	ASSERT_EQ(certificates.size(), 1u);

	std::expected<void, std::error_code> const verified{ VerifyAlone(certificates.front()) };

	ASSERT_FALSE(verified.has_value());
	EXPECT_EQ(verified.error(), std::errc::not_supported);
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
