#include "poller.hpp"
#include <tge/logging/loggers.hpp>
#include <tge/platform.hpp>

#if defined(TGE_PLATFORM_LINUX)
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <span>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif // defined(TGE_PLATFORM_LINUX)

namespace Tge::Threading
{
#if defined(TGE_PLATFORM_LINUX)
namespace
{
constexpr uint64_t WakeKey{ std::numeric_limits<uint64_t>::max() };
constexpr size_t MaxEventsPerWait{ 32 };
} // namespace

//////////////////////////////////////////////////////////////////////////
bool CPoller::Initialize()
{
	bool isReady{ false };
	m_pollDescriptor = epoll_create1(EPOLL_CLOEXEC);

	if (m_pollDescriptor >= 0)
	{
		m_wakeDescriptor = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

		if (m_wakeDescriptor >= 0)
		{
			isReady = Add(m_wakeDescriptor, WakeKey);
		}
		else
		{
			gLog.Error("Event loop: eventfd failed: {}", std::strerror(errno));
		}
	}
	else
	{
		gLog.Error("Event loop: epoll_create1 failed: {}", std::strerror(errno));
	}

	if (!isReady)
	{
		Terminate();
	}

	return isReady;
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Terminate()
{
	if (m_wakeDescriptor >= 0 && close(m_wakeDescriptor) != 0)
	{
		gLog.Warning("Event loop: cannot close the wake signal: {}", std::strerror(errno));
	}

	if (m_pollDescriptor >= 0 && close(m_pollDescriptor) != 0)
	{
		gLog.Warning("Event loop: cannot close epoll: {}", std::strerror(errno));
	}

	m_wakeDescriptor = -1;
	m_pollDescriptor = -1;
}

//////////////////////////////////////////////////////////////////////////
bool CPoller::Add(int descriptor, uint64_t key)
{
	epoll_event event{};
	event.events = EPOLLIN;
	event.data.u64 = key;
	bool const isAdded{ epoll_ctl(m_pollDescriptor, EPOLL_CTL_ADD, descriptor, &event) == 0 };

	if (!isAdded)
	{
		gLog.Error("Event loop: cannot watch descriptor {}: {}", descriptor, std::strerror(errno));
	}

	return isAdded;
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Remove(int descriptor)
{
	if (epoll_ctl(m_pollDescriptor, EPOLL_CTL_DEL, descriptor, nullptr) != 0)
	{
		gLog.Warning("Event loop: cannot unwatch descriptor {}: {}", descriptor, std::strerror(errno));
	}
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Wake()
{
	uint64_t const increment{ 1 };

	if (write(m_wakeDescriptor, &increment, sizeof(increment)) < 0)
	{
		gLog.Error("Event loop: cannot wake the loop: {}", std::strerror(errno));
	}
}

//////////////////////////////////////////////////////////////////////////
bool CPoller::Wait(std::optional<std::chrono::milliseconds> timeout, std::vector<uint64_t>& readyKeys)
{
	int timeoutMs{ -1 };

	if (timeout.has_value())
	{
		timeoutMs = static_cast<int>(std::min<std::chrono::milliseconds::rep>(timeout->count(), std::numeric_limits<int>::max()));
	}

	std::array<epoll_event, MaxEventsPerWait> events{};
	int const numEvents{ epoll_wait(m_pollDescriptor, events.data(), static_cast<int>(events.size()), timeoutMs) };
	int const waitError{ (numEvents < 0) ? errno : 0 };
	bool const isWaiting{ numEvents >= 0 || waitError == EINTR };

	size_t const numReady{ static_cast<size_t>(std::max(numEvents, 0)) };

	readyKeys.clear();

	for (epoll_event const& event : std::span{ events }.first(numReady))
	{
		uint64_t const key{ event.data.u64 };

		if (key == WakeKey)
		{
			uint64_t count{ 0 };

			if (read(m_wakeDescriptor, &count, sizeof(count)) < 0 && errno != EAGAIN)
			{
				gLog.Error("Event loop: cannot reset the wake signal: {}", std::strerror(errno));
			}
		}
		else
		{
			readyKeys.push_back(key);
		}
	}

	if (!isWaiting)
	{
		gLog.Error("Event loop stopped: epoll_wait failed: {}", std::strerror(waitError));
	}

	return isWaiting;
}
#else
//////////////////////////////////////////////////////////////////////////
bool CPoller::Initialize()
{
	gLog.Error("Event loop: not available on this platform");
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Terminate()
{
}

//////////////////////////////////////////////////////////////////////////
bool CPoller::Add(int, uint64_t)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Remove(int)
{
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Wake()
{
}

//////////////////////////////////////////////////////////////////////////
bool CPoller::Wait(std::optional<std::chrono::milliseconds>, std::vector<uint64_t>& readyKeys)
{
	readyKeys.clear();
	return false;
}
#endif // defined(TGE_PLATFORM_LINUX)
} // namespace Tge::Threading
