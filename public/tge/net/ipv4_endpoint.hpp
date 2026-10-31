#pragma once

#include <cstdint>

namespace Tge::Net
{
struct SIpv4Endpoint final
{
	uint32_t address{ 0 };
	uint16_t port{ 0 };
};

constexpr bool operator==(SIpv4Endpoint const& lhs, SIpv4Endpoint const& rhs)
{
	return lhs.address == rhs.address && lhs.port == rhs.port;
}
} // namespace Tge::Net
