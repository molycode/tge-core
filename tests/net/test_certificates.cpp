#include "pem.hpp"
#include <gtest/gtest.h>
#include <tge/net/root_certificates.hpp>
#include <tge/platform.hpp>
#include <cstddef>
#include <expected>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(TGE_PLATFORM_LINUX)
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#endif // defined(TGE_PLATFORM_LINUX)

using namespace Tge::Net;

namespace
{
using Bytes = std::vector<std::byte>;

Bytes ToBytes(std::initializer_list<unsigned char> values)
{
	Bytes bytes{};

	for (unsigned char const value : values)
	{
		bytes.push_back(static_cast<std::byte>(value));
	}

	return bytes;
}

constexpr std::string_view TwoBlocks{
	"Some bundle header text\n"
	"-----BEGIN CERTIFICATE-----\n"
	"AAECAwQ=\n"
	"-----END CERTIFICATE-----\n"
	"between\n"
	"-----BEGIN CERTIFICATE-----\r\n"
	"/w==\r\n"
	"-----END CERTIFICATE-----\r\n" };

#if defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
std::expected<std::vector<Bytes>, std::error_code> ReadWithCertFile(std::filesystem::path const& file)
{
	char const* const pPreviousFile{ std::getenv("SSL_CERT_FILE") };
	char const* const pPreviousFolder{ std::getenv("SSL_CERT_DIR") };
	std::optional<std::string> const previousFile{ (pPreviousFile != nullptr) ? std::optional<std::string>{ pPreviousFile } : std::nullopt };
	std::optional<std::string> const previousFolder{ (pPreviousFolder != nullptr) ? std::optional<std::string>{ pPreviousFolder } : std::nullopt };

	setenv("SSL_CERT_FILE", file.c_str(), 1);
	unsetenv("SSL_CERT_DIR");

	std::expected<std::vector<Bytes>, std::error_code> const certificates{ ReadSystemRootCertificates() };

	if (previousFile.has_value())
	{
		setenv("SSL_CERT_FILE", previousFile->c_str(), 1);
	}
	else
	{
		unsetenv("SSL_CERT_FILE");
	}

	if (previousFolder.has_value())
	{
		setenv("SSL_CERT_DIR", previousFolder->c_str(), 1);
	}

	return certificates;
}
#endif // defined(TGE_PLATFORM_LINUX)
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST(Certificates, DecodesEachPemBlock)
{
	std::vector<Bytes> const certificates{ DecodeCertificates(TwoBlocks) };

	ASSERT_EQ(certificates.size(), 2u);
	EXPECT_EQ(certificates[0], ToBytes({ 0x00, 0x01, 0x02, 0x03, 0x04 }));
	EXPECT_EQ(certificates[1], ToBytes({ 0xFF }));
}

//////////////////////////////////////////////////////////////////////////
TEST(Certificates, JoinsABlockSplitOverLines)
{
	std::vector<Bytes> const certificates{ DecodeCertificates("-----BEGIN CERTIFICATE-----\nAAEC\r\nAwQ=\n-----END CERTIFICATE-----\n") };

	ASSERT_EQ(certificates.size(), 1u);
	EXPECT_EQ(certificates[0], ToBytes({ 0x00, 0x01, 0x02, 0x03, 0x04 }));
}

//////////////////////////////////////////////////////////////////////////
TEST(Certificates, SkipsABlockThatIsNotBase64)
{
	EXPECT_TRUE(DecodeCertificates("-----BEGIN CERTIFICATE-----\n#not base64#\n-----END CERTIFICATE-----\n").empty());
}

//////////////////////////////////////////////////////////////////////////
TEST(Certificates, TakesContentWithoutPemHeaderAsOneDerCertificate)
{
	constexpr std::string_view Der{ "\x30\x03\x02\x01\x05", 5 };
	std::vector<Bytes> const certificates{ DecodeCertificates(Der) };

	ASSERT_EQ(certificates.size(), 1u);
	EXPECT_EQ(certificates[0], ToBytes({ 0x30, 0x03, 0x02, 0x01, 0x05 }));
}

//////////////////////////////////////////////////////////////////////////
TEST(Certificates, IgnoresTextWithoutPemHeader)
{
	EXPECT_TRUE(DecodeCertificates("not a certificate\n").empty());
}

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
TEST(RootCertificates, WindowsStoreGivesDerCertificates)
{
	std::expected<std::vector<Bytes>, std::error_code> const certificates{ ReadSystemRootCertificates() };

	ASSERT_TRUE(certificates.has_value());
	EXPECT_FALSE(certificates->empty());

	for (Bytes const& certificate : *certificates)
	{
		ASSERT_FALSE(certificate.empty());
		EXPECT_EQ(certificate.front(), std::byte{ 0x30 });
	}
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
TEST(RootCertificates, ReadsTheBundleSslCertFileNames)
{
	std::error_code error{};
	std::filesystem::path const bundle{ std::filesystem::temp_directory_path(error) / "tge-net-tests-bundle.pem" };

	ASSERT_EQ(error, std::error_code{});

	std::ofstream{ bundle, std::ios::binary } << TwoBlocks;

	std::expected<std::vector<Bytes>, std::error_code> const certificates{ ReadWithCertFile(bundle) };

	std::filesystem::remove(bundle, error);

	ASSERT_TRUE(certificates.has_value());
	EXPECT_EQ(*certificates, DecodeCertificates(TwoBlocks));
	EXPECT_EQ(error, std::error_code{});
}

//////////////////////////////////////////////////////////////////////////
TEST(RootCertificates, FindsNoneInAFolderSslCertFileNames)
{
	std::error_code error{};
	std::filesystem::path const folder{ std::filesystem::temp_directory_path(error) };

	ASSERT_EQ(error, std::error_code{});

	std::expected<std::vector<Bytes>, std::error_code> const certificates{ ReadWithCertFile(folder) };

	ASSERT_FALSE(certificates.has_value());
	EXPECT_EQ(certificates.error(), std::errc::invalid_argument);
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
