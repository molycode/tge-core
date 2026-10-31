#include <tge/net/socket_library.hpp>
#include "native_socket.hpp"
#include <tge/assert.hpp>

namespace Tge::Net
{
//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> CSocketLibrary::Initialize()
{
	TGE_ASSERT(!m_isStarted, "Socket library initialized twice");

	std::expected<void, std::error_code> result{};

#if defined(TGE_PLATFORM_WINDOWS)
	WSADATA data{};
	int const started{ WSAStartup(MAKEWORD(2, 2), &data) };

	if (started != 0)
	{
		result = std::unexpected{ std::error_code{ started, std::system_category() } };
	}
#endif // defined(TGE_PLATFORM_WINDOWS)

	m_isStarted = result.has_value();

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CSocketLibrary::Terminate()
{
#if defined(TGE_PLATFORM_WINDOWS)
	if (m_isStarted)
	{
		WSACleanup();
	}
#endif // defined(TGE_PLATFORM_WINDOWS)

	m_isStarted = false;
}
} // namespace Tge::Net
