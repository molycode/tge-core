#include <tge/net/verify_server_chain.hpp>
#include <tge/platform.hpp>

#if defined(TGE_PLATFORM_WINDOWS)
#include <array>
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS
#include <windows.h>
#include <wincrypt.h>
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::Net
{
#if defined(TGE_PLATFORM_WINDOWS)
namespace
{
// Building the chain may fetch a root from Windows Update, and the calling thread waits for it.
constexpr DWORD UrlRetrievalTimeoutMs{ 5000 };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastErrorCode()
{
	return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
std::wstring ToWide(std::string_view text)
{
	std::wstring wide{};
	int const size{ MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0) };

	if (size > 0)
	{
		wide.resize(static_cast<size_t>(size));
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
	}

	return wide;
}

//////////////////////////////////////////////////////////////////////////
// The SSL policy checks the whole chain's trust and the host name; its verdict is a Windows error code.
std::expected<void, std::error_code> CheckServerPolicy(PCCERT_CHAIN_CONTEXT pChain, std::string_view hostName)
{
	std::wstring wideHostName{ ToWide(hostName) };
	SSL_EXTRA_CERT_CHAIN_POLICY_PARA extra{};
	CERT_CHAIN_POLICY_PARA policy{};
	CERT_CHAIN_POLICY_STATUS status{};
	std::expected<void, std::error_code> result{};

	extra.cbSize = sizeof(extra);
	extra.dwAuthType = AUTHTYPE_SERVER;
	extra.pwszServerName = wideHostName.data();
	policy.cbSize = sizeof(policy);
	policy.pvExtraPolicyPara = &extra;
	status.cbSize = sizeof(status);

	if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, pChain, &policy, &status) == FALSE)
	{
		result = std::unexpected{ GetLastErrorCode() };
	}
	else if (status.dwError != 0)
	{
		result = std::unexpected{ std::error_code{ static_cast<int>(status.dwError), std::system_category() } };
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
// No revocation check, as Mbed TLS makes none either.
std::expected<void, std::error_code> BuildAndCheck(PCCERT_CONTEXT pServer, HCERTSTORE sent, std::string_view hostName)
{
	std::string serverAuth{ szOID_PKIX_KP_SERVER_AUTH };
	std::array<LPSTR, 1> usages{ serverAuth.data() };
	CERT_CHAIN_PARA parameters{};
	PCCERT_CHAIN_CONTEXT pChain{ nullptr };
	std::expected<void, std::error_code> result{};

	parameters.cbSize = sizeof(parameters);
	parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
	parameters.RequestedUsage.Usage.cUsageIdentifier = static_cast<DWORD>(usages.size());
	parameters.RequestedUsage.Usage.rgpszUsageIdentifier = usages.data();
	parameters.dwUrlRetrievalTimeout = UrlRetrievalTimeoutMs;

	if (CertGetCertificateChain(nullptr, pServer, nullptr, sent, &parameters, 0, nullptr, &pChain) != FALSE)
	{
		result = CheckServerPolicy(pChain, hostName);
		CertFreeCertificateChain(pChain);
	}
	else
	{
		result = std::unexpected{ GetLastErrorCode() };
	}

	return result;
}
} // namespace
#endif // defined(TGE_PLATFORM_WINDOWS)

//////////////////////////////////////////////////////////////////////////
// Windows fills its ROOT store lazily, so a root ReadSystemRootCertificates did not see may still be trusted; its chain
// engine fetches it. Linux's bundles hold every root there is, so there is nothing more to ask.
std::expected<void, std::error_code> VerifyServerChain(std::span<std::span<std::byte const> const> chain, std::string_view hostName)
{
	std::expected<void, std::error_code> result{ std::unexpected{ std::make_error_code(std::errc::invalid_argument) } };

#if defined(TGE_PLATFORM_WINDOWS)
	if (!chain.empty())
	{
		HCERTSTORE const sent{ CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, 0, nullptr) };
		std::error_code const storeError{ (sent == nullptr) ? GetLastErrorCode() : std::error_code{} };
		PCCERT_CONTEXT const pServer{ CertCreateCertificateContext(X509_ASN_ENCODING, reinterpret_cast<BYTE const*>(chain.front().data()),
			static_cast<DWORD>(chain.front().size())) };
		std::error_code const serverError{ (pServer == nullptr) ? GetLastErrorCode() : std::error_code{} };

		if (sent != nullptr && pServer != nullptr)
		{
			// One the server sent that cannot be read is left out; the chain is then built without it, or not at all.
			for (std::span<std::byte const> const certificate : chain.subspan(1))
			{
				static_cast<void>(CertAddEncodedCertificateToStore(sent, X509_ASN_ENCODING, reinterpret_cast<BYTE const*>(certificate.data()),
					static_cast<DWORD>(certificate.size()), CERT_STORE_ADD_ALWAYS, nullptr));
			}

			result = BuildAndCheck(pServer, sent, hostName);
		}
		else
		{
			result = std::unexpected{ (sent == nullptr) ? storeError : serverError };
		}

		if (pServer != nullptr)
		{
			CertFreeCertificateContext(pServer);
		}

		if (sent != nullptr)
		{
			CertCloseStore(sent, 0);
		}
	}
#elif defined(TGE_PLATFORM_LINUX)
	result = std::unexpected{ std::make_error_code(std::errc::not_supported) };
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return result;
}
} // namespace Tge::Net
