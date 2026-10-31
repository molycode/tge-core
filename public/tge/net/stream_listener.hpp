#pragma once

#include <tge/net/ipv4_endpoint.hpp>
#include <tge/non_copyable.hpp>
#include <tge/threading/waitable.hpp>
#include <cstdint>
#include <expected>
#include <limits>
#include <system_error>

namespace Tge::Net
{
class CStreamSocket;

class CStreamListener final : private SNoCopyNoMove
{
public:

	CStreamListener() = default;
	~CStreamListener() = default;

	std::expected<void, std::error_code> Initialize(SIpv4Endpoint local);
	void Terminate();

	std::expected<bool, std::error_code> Accept(CStreamSocket& connection) const;

	std::expected<SIpv4Endpoint, std::error_code> GetLocalEndpoint() const;
	Threading::SWaitable GetWaitable() const;

private:

	uintptr_t m_socket{ std::numeric_limits<uintptr_t>::max() };
};
} // namespace Tge::Net
