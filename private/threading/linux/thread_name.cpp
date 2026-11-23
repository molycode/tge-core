#include "threading/os_thread_name.hpp"

#include <cstring>
#include <pthread.h>

namespace Tge::Threading
{
//////////////////////////////////////////////////////////////////////////
void SetOsThreadName(std::string_view name)
{
	// Linux caps a thread's name at 15 characters; the profiler hook still receives the whole name.
	constexpr size_t MaxOsNameLength{ 15u };
	char osName[MaxOsNameLength + 1u]{};
	size_t const copyLength{ (name.size() < MaxOsNameLength) ? name.size() : MaxOsNameLength };

	std::memcpy(osName, name.data(), copyLength);
	pthread_setname_np(pthread_self(), osName);
}
} // namespace Tge::Threading
