#include "threading/os_thread_name.hpp"

namespace Tge::Threading
{
//////////////////////////////////////////////////////////////////////////
void SetOsThreadName(std::string_view name)
{
	// The web has no operating system that shows thread names; the profiler hook still receives the name.
}
} // namespace Tge::Threading
