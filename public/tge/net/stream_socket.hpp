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
class CStreamSocket final : private SNoCopyNoMove
{
public:

	CStreamSocket() = default;
	~CStreamSocket() = default;

	std::expected<void, std::error_code> Initialize();
	void Terminate();

	std::expected<void, std::error_code> Connect(SIpv4Endpoint peer) const;
	std::expected<bool, std::error_code> PollConnected() const;
	std::expected<size_t, std::error_code> Send(std::span<std::byte const> bytes) const;
	std::expected<size_t, std::error_code> Receive(std::span<std::byte> buffer) const;

	std::expected<void, std::error_code> SetBlocking() const;
	std::expected<void, std::error_code> SetNoDelay() const;
	Threading::SWaitable GetWaitable() const;

private:

	friend class CStreamListener;

	uintptr_t m_socket{ std::numeric_limits<uintptr_t>::max() };
};
} // namespace Tge::Net
