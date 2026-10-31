#pragma once

#include "native_socket.hpp"
#include <string>

#if defined(TGE_PLATFORM_LINUX)
#include <netdb.h>
#endif // defined(TGE_PLATFORM_LINUX)

namespace Tge::Net
{
struct SNativeLookup final
{
#if defined(TGE_PLATFORM_WINDOWS)
	std::wstring host{};
	ADDRINFOEXW hints{};
	ADDRINFOEXW* pResult{ nullptr };
	OVERLAPPED overlapped{};
	HANDLE cancel{ nullptr };
	bool isComplete{ false };
#elif defined(TGE_PLATFORM_LINUX)
	std::string host{};
	addrinfo hints{};
	gaicb request{};
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
};
} // namespace Tge::Net
