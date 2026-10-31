#pragma once

#include <tge/net/ipv4_endpoint.hpp>
#include <tge/non_copyable.hpp>
#include <tge/threading/waitable.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <system_error>

namespace Tge::Net
{
class CUdpSocket final : private SNoCopyNoMove
{
public:

	CUdpSocket() = default;
	~CUdpSocket() = default;

	std::expected<void, std::error_code> Initialize(SIpv4Endpoint local);
	void Terminate();

	std::expected<void, std::error_code> SetReceiveBufferSize(uint32_t numBytes) const;
	std::expected<void, std::error_code> DisableConnectionResetReports() const;
	std::expected<void, std::error_code> Connect(SIpv4Endpoint peer) const;
	std::expected<void, std::error_code> Send(SIpv4Endpoint destination, std::span<std::byte const> datagram) const;
	std::expected<size_t, std::error_code> Receive(std::span<std::byte> buffer, SIpv4Endpoint& source) const;

	std::expected<SIpv4Endpoint, std::error_code> GetLocalEndpoint() const;
	Threading::SWaitable GetWaitable() const;

private:

	uintptr_t m_socket{ std::numeric_limits<uintptr_t>::max() };
};
} // namespace Tge::Net
