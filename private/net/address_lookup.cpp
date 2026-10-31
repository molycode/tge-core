#include <tge/net/address_lookup.hpp>
#include "lookup_error_category.hpp"
#include "native_lookup.hpp"
#include <tge/assert.hpp>
#include <array>

#if defined(TGE_PLATFORM_LINUX)
#include <arpa/inet.h>
#endif // defined(TGE_PLATFORM_LINUX)

namespace Tge::Net
{
namespace
{
#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
std::wstring ToWide(std::string_view text)
{
	std::wstring wide{};
	int const numWide{ MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0) };

	wide.resize(static_cast<size_t>(numWide));
	MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), numWide);

	return wide;
}

//////////////////////////////////////////////////////////////////////////
std::expected<uint32_t, std::error_code> TakeFirstAddress(SNativeLookup const& lookup, int status)
{
	std::expected<uint32_t, std::error_code> result{ std::unexpected{ std::error_code{ status, std::system_category() } } };

	if (status == NO_ERROR)
	{
		result = std::unexpected{ std::error_code{ WSAHOST_NOT_FOUND, std::system_category() } };

		for (ADDRINFOEXW const* pEntry{ lookup.pResult }; pEntry != nullptr && !result.has_value(); pEntry = pEntry->ai_next)
		{
			if (pEntry->ai_family == AF_INET)
			{
				result = ntohl(reinterpret_cast<sockaddr_in const*>(pEntry->ai_addr)->sin_addr.s_addr);
			}
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void Release(SNativeLookup& lookup)
{
	if (lookup.pResult != nullptr)
	{
		FreeAddrInfoExW(lookup.pResult);
	}

	if (lookup.overlapped.hEvent != nullptr)
	{
		CloseHandle(lookup.overlapped.hEvent);
	}
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
std::error_code ToLookupError(int status)
{
	return std::error_code{ status, GetLookupErrorCategory() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<uint32_t, std::error_code> TakeFirstAddress(SNativeLookup& lookup, int status)
{
	std::expected<uint32_t, std::error_code> result{ std::unexpected{ ToLookupError(status) } };

	if (status == 0)
	{
		result = std::unexpected{ ToLookupError(EAI_NONAME) };

		for (addrinfo const* pEntry{ lookup.request.ar_result }; pEntry != nullptr && !result.has_value(); pEntry = pEntry->ai_next)
		{
			if (pEntry->ai_family == AF_INET)
			{
				result = ntohl(reinterpret_cast<sockaddr_in const*>(pEntry->ai_addr)->sin_addr.s_addr);
			}
		}

		freeaddrinfo(lookup.request.ar_result);
		lookup.request.ar_result = nullptr;
	}

	return result;
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
CAddressLookup::CAddressLookup() = default;

//////////////////////////////////////////////////////////////////////////
CAddressLookup::~CAddressLookup() = default;

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CAddressLookup::Start(std::string_view host)
{
	TGE_ASSERT(m_pLookup == nullptr, "Address lookup started while one is running");

	auto pLookup{ std::make_unique<SNativeLookup>() };
	std::expected<void, std::error_code> result{};

#if defined(TGE_PLATFORM_WINDOWS)
	pLookup->host = ToWide(host);
	pLookup->hints.ai_family = AF_INET;
	pLookup->hints.ai_socktype = SOCK_STREAM;
	pLookup->overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

	int const status{ (pLookup->overlapped.hEvent != nullptr)
		? GetAddrInfoExW(pLookup->host.c_str(), nullptr, NS_ALL, nullptr, &pLookup->hints, &pLookup->pResult, nullptr, &pLookup->overlapped, nullptr, &pLookup->cancel)
		: static_cast<int>(GetLastError()) };

	pLookup->isComplete = status == NO_ERROR;

	if (status == NO_ERROR || status == WSA_IO_PENDING)
	{
		m_pLookup = std::move(pLookup);
	}
	else
	{
		Release(*pLookup);
		result = std::unexpected{ std::error_code{ status, std::system_category() } };
	}
#elif defined(TGE_PLATFORM_LINUX)
	pLookup->host = std::string{ host };
	pLookup->hints.ai_family = AF_INET;
	pLookup->hints.ai_socktype = SOCK_STREAM;
	pLookup->request.ar_name = pLookup->host.c_str();
	pLookup->request.ar_request = &pLookup->hints;

	std::array<gaicb*, 1> requests{ &pLookup->request };
	// No notification: it would run on a glibc thread and race the owner, who polls instead.
	int const status{ getaddrinfo_a(GAI_NOWAIT, requests.data(), static_cast<int>(requests.size()), nullptr) };

	if (status == 0)
	{
		m_pLookup = std::move(pLookup);
	}
	else
	{
		result = std::unexpected{ ToLookupError(status) };
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return result;
}

//////////////////////////////////////////////////////////////////////////
std::optional<std::expected<uint32_t, std::error_code>> CAddressLookup::Poll()
{
	TGE_ASSERT(m_pLookup != nullptr, "Poll of an address lookup that is not running");

	std::optional<std::expected<uint32_t, std::error_code>> result{};

#if defined(TGE_PLATFORM_WINDOWS)
	int const status{ m_pLookup->isComplete ? NO_ERROR : GetAddrInfoExOverlappedResult(&m_pLookup->overlapped) };

	if (status != WSAEINPROGRESS)
	{
		result = TakeFirstAddress(*m_pLookup, status);
		Release(*m_pLookup);
		m_pLookup.reset();
	}
#elif defined(TGE_PLATFORM_LINUX)
	int const status{ gai_error(&m_pLookup->request) };

	if (status != EAI_INPROGRESS)
	{
		result = TakeFirstAddress(*m_pLookup, status);
		m_pLookup.reset();
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CAddressLookup::Terminate()
{
	if (m_pLookup != nullptr)
	{
#if defined(TGE_PLATFORM_WINDOWS)
		bool const isPending{ !m_pLookup->isComplete && GetAddrInfoExOverlappedResult(&m_pLookup->overlapped) == WSAEINPROGRESS };

		if (isPending)
		{
			// A cancel completes later, and the system writes into the lookup then, so the lookup is left to it.
			GetAddrInfoExCancel(&m_pLookup->cancel);
			static_cast<void>(m_pLookup.release());
		}
		else
		{
			Release(*m_pLookup);
			m_pLookup.reset();
		}
#elif defined(TGE_PLATFORM_LINUX)
		// glibc keeps writing into a request it could not cancel, so that one is left to it.
		if (gai_cancel(&m_pLookup->request) == EAI_NOTCANCELED)
		{
			static_cast<void>(m_pLookup.release());
		}
		else
		{
			if (m_pLookup->request.ar_result != nullptr)
			{
				freeaddrinfo(m_pLookup->request.ar_result);
			}

			m_pLookup.reset();
		}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
	}
}
} // namespace Tge::Net
