#include <tge/net/root_certificates.hpp>
#include "pem.hpp"
#include <tge/platform.hpp>

#if defined(TGE_PLATFORM_WINDOWS)
#include <algorithm>
#include <cstring>
#include <span>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <wincrypt.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Net
{
namespace
{
using Certificates = std::vector<std::vector<std::byte>>;

#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
// The ROOT store also trusts roots for other uses alone, such as code signing; the Linux bundles hold TLS roots.
bool IsForServers(PCCERT_CONTEXT pCertificate)
{
	DWORD size{ 0 };
	bool isForServers{ false };

	if (CertGetEnhancedKeyUsage(pCertificate, 0, nullptr, &size) != 0)
	{
		std::vector<std::byte> buffer{};
		buffer.resize(size);
		CERT_ENHKEY_USAGE* const pUsage{ reinterpret_cast<CERT_ENHKEY_USAGE*>(buffer.data()) };

		if (CertGetEnhancedKeyUsage(pCertificate, 0, pUsage, &size) != 0)
		{
			std::span<LPSTR const> const usages{ pUsage->rgpszUsageIdentifier, pUsage->cUsageIdentifier };

			// No usage listed means every use when the reason is CRYPT_E_NOT_FOUND, and none otherwise.
			isForServers = usages.empty() ? GetLastError() == static_cast<DWORD>(CRYPT_E_NOT_FOUND)
				: std::ranges::any_of(usages, [](LPSTR pUsageId) { return std::strcmp(pUsageId, szOID_PKIX_KP_SERVER_AUTH) == 0; });
		}
	}

	return isForServers;
}
#elif defined(TGE_PLATFORM_LINUX)
// Go's crypto/x509 list, in its order: Debian, Ubuntu, Arch, Gentoo and Mint first, then the Red Hat family and SUSE.
constexpr std::array<char const*, 6> BundleFiles{
	"/etc/ssl/certs/ca-certificates.crt",
	"/etc/pki/tls/certs/ca-bundle.crt",
	"/etc/ssl/ca-bundle.pem",
	"/etc/pki/tls/cacert.pem",
	"/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
	"/etc/ssl/cert.pem"
};
constexpr std::array<char const*, 2> BundleFolders{ "/etc/ssl/certs", "/etc/pki/tls/certs" };

//////////////////////////////////////////////////////////////////////////
// istream::read, not istreambuf_iterator: libstdc++ throws on a read error such as a directory's, and without
// exceptions that aborts.
Certificates ReadFile(std::filesystem::path const& path)
{
	std::error_code error{};
	bool const isFile{ std::filesystem::is_regular_file(path, error) };
	std::uintmax_t const size{ isFile ? std::filesystem::file_size(path, error) : 0 };
	std::string content{};

	if (isFile && error == std::error_code{})
	{
		std::ifstream stream{ path, std::ios::binary };

		content.resize(static_cast<size_t>(size));
		stream.read(content.data(), static_cast<std::streamsize>(content.size()));
		content.resize(static_cast<size_t>(stream.gcount()));
	}

	return DecodeCertificates(content);
}

//////////////////////////////////////////////////////////////////////////
// increment(error), not operator++, which throws when the folder cannot be read on.
Certificates ReadFolder(std::filesystem::path const& folder)
{
	Certificates certificates{};
	std::error_code error{};

	for (std::filesystem::directory_iterator entry{ folder, error }, end{}; error == std::error_code{} && entry != end; entry.increment(error))
	{
		Certificates read{ ReadFile(entry->path()) };
		certificates.insert(certificates.end(), std::make_move_iterator(read.begin()), std::make_move_iterator(read.end()));
	}

	return certificates;
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
std::expected<std::vector<std::vector<std::byte>>, std::error_code> ReadSystemRootCertificates()
{
	std::expected<Certificates, std::error_code> result{};

#if defined(TGE_PLATFORM_WINDOWS)
	HCERTSTORE const store{ CertOpenSystemStoreW(0, L"ROOT") };

	if (store != nullptr)
	{
		Certificates certificates{};
		PCCERT_CONTEXT pCertificate{ CertEnumCertificatesInStore(store, nullptr) };

		while (pCertificate != nullptr)
		{
			if (IsForServers(pCertificate))
			{
				std::byte const* const pEncoded{ reinterpret_cast<std::byte const*>(pCertificate->pbCertEncoded) };
				certificates.emplace_back(pEncoded, pEncoded + pCertificate->cbCertEncoded);
			}

			pCertificate = CertEnumCertificatesInStore(store, pCertificate);
		}

		CertCloseStore(store, 0);
		result = certificates.empty() ? std::expected<Certificates, std::error_code>{ std::unexpected{ std::make_error_code(std::errc::no_such_file_or_directory) } }
			: std::expected<Certificates, std::error_code>{ std::move(certificates) };
	}
	else
	{
		result = std::unexpected{ std::error_code{ static_cast<int>(GetLastError()), std::system_category() } };
	}
#elif defined(TGE_PLATFORM_LINUX)
	char const* const pEnvironmentFile{ std::getenv("SSL_CERT_FILE") };
	char const* const pEnvironmentFolder{ std::getenv("SSL_CERT_DIR") };
	Certificates certificates{};

	// Either variable replaces the bundles. The folder is one path, not split at ':', as Mbed TLS read it.
	if (pEnvironmentFile != nullptr || pEnvironmentFolder != nullptr)
	{
		if (pEnvironmentFile != nullptr)
		{
			certificates = ReadFile(pEnvironmentFile);
		}

		if (certificates.empty() && pEnvironmentFolder != nullptr)
		{
			certificates = ReadFolder(pEnvironmentFolder);
		}

		result = certificates.empty() ? std::expected<Certificates, std::error_code>{ std::unexpected{ std::make_error_code(std::errc::invalid_argument) } }
			: std::expected<Certificates, std::error_code>{ std::move(certificates) };
	}
	else
	{
		for (char const* const pPath : BundleFiles)
		{
			if (certificates.empty())
			{
				certificates = ReadFile(pPath);
			}
		}

		for (char const* const pPath : BundleFolders)
		{
			if (certificates.empty())
			{
				certificates = ReadFolder(pPath);
			}
		}

		result = certificates.empty() ? std::expected<Certificates, std::error_code>{ std::unexpected{ std::make_error_code(std::errc::no_such_file_or_directory) } }
			: std::expected<Certificates, std::error_code>{ std::move(certificates) };
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return result;
}
} // namespace Tge::Net
