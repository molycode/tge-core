#include <tge/net/wait_for_readable.hpp>
#include "native_socket.hpp"
#include <tge/assert.hpp>

namespace Tge::Net
{
//////////////////////////////////////////////////////////////////////////
std::expected<bool, std::error_code> WaitForReadable(Threading::SWaitable socket, std::chrono::milliseconds timeout)
{
	TGE_ASSERT(socket.kind == Threading::EWaitableKind::Socket, "WaitForReadable waits on sockets only");

	return PollSocket(socket.native, EReadiness::Readable, timeout);
}
} // namespace Tge::Net
