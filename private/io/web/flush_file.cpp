#include "io/posix/flush_file.hpp"

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
// Emscripten's fsync stores the whole mount by itself, and a worker waits in it for the browser's event loop, which a
// main thread waiting for that worker never turns. The frame's flush makes the file last.
std::error_code FlushFile(int file)
{
	return {};
}
} // namespace Tge::IO
