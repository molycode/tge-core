#include "threading/thread_support.hpp"

#include <limits>
#include <thread>

namespace Tge::Threading
{
//////////////////////////////////////////////////////////////////////////
size_t GetMaxWorkerThreads()
{
	return std::numeric_limits<size_t>::max();
}

//////////////////////////////////////////////////////////////////////////
size_t GetAutoWorkerThreads()
{
	size_t const numHardwareThreads{ std::thread::hardware_concurrency() };

	return (numHardwareThreads > 0u) ? numHardwareThreads : 4u;
}
} // namespace Tge::Threading
