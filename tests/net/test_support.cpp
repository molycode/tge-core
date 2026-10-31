#include "test_support.hpp"
#include <thread>

namespace Tge::Net::Tests
{
//////////////////////////////////////////////////////////////////////////
std::span<std::byte const> AsBytes(std::string_view text)
{
	return std::as_bytes(std::span{ text });
}

//////////////////////////////////////////////////////////////////////////
std::string_view AsText(std::span<std::byte const> bytes)
{
	return std::string_view{ reinterpret_cast<char const*>(bytes.data()), bytes.size() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<bool, std::error_code> WaitUntilConnected(CStreamSocket const& socket)
{
	auto const deadline{ std::chrono::steady_clock::now() + Patience };
	std::expected<bool, std::error_code> connected{ socket.PollConnected() };

	while (connected.has_value() && !*connected && std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
		connected = socket.PollConnected();
	}

	return connected;
}
} // namespace Tge::Net::Tests
