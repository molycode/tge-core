#include "io/posix/flush_file.hpp"
#include <cerrno>
#include <unistd.h>

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
std::error_code FlushFile(int file)
{
	return (::fsync(file) == 0) ? std::error_code{} : std::error_code{ errno, std::system_category() };
}
} // namespace Tge::IO
