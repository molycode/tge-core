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
#elif defined(TGE_PLATFORM_WINDOWS)
#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <system_error>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <winsock2.h>
#include <windows.h>
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
			isReady = Add(SWaitable{ static_cast<uintptr_t>(m_wakeDescriptor), EWaitableKind::Event }, WakeKey);
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
bool CPoller::Add(SWaitable waitable, uint64_t key)
{
	int const descriptor{ static_cast<int>(waitable.native) };
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
void CPoller::Remove(SWaitable waitable)
{
	int const descriptor{ static_cast<int>(waitable.native) };

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
#elif defined(TGE_PLATFORM_WINDOWS)
namespace
{
constexpr size_t MaxSocketsPerPoll{ 64 };
constexpr DWORD SocketEventIndex{ 1 };
constexpr DWORD NumOwnHandles{ 2 };
constexpr size_t MaxEvents{ MAXIMUM_WAIT_OBJECTS - NumOwnHandles };
// FD_CONNECT so a refused connect wakes the wait; a connect that succeeds wakes it for nothing.
constexpr long SocketEvents{ FD_READ | FD_ACCEPT | FD_CLOSE | FD_CONNECT };

//////////////////////////////////////////////////////////////////////////
std::string DescribeError(int error)
{
	return std::system_category().message(error);
}

//////////////////////////////////////////////////////////////////////////
DWORD ToTimeoutMs(std::optional<std::chrono::milliseconds> timeout)
{
	DWORD timeoutMs{ INFINITE };

	if (timeout.has_value())
	{
		timeoutMs = static_cast<DWORD>(std::min(timeout->count(), static_cast<std::chrono::milliseconds::rep>(INFINITE - 1)));
	}

	return timeoutMs;
}

//////////////////////////////////////////////////////////////////////////
bool PollChunk(std::span<WSAPOLLFD> polls, std::span<uint64_t const> keys, std::vector<uint64_t>& readyKeys)
{
	bool const isPolled{ WSAPoll(polls.data(), static_cast<ULONG>(polls.size()), 0) != SOCKET_ERROR };

	if (isPolled)
	{
		for (size_t index{ 0 }; index < polls.size(); ++index)
		{
			// Hang-up and error count as readable, as epoll reports them.
			if ((polls[index].revents & (POLLRDNORM | POLLHUP | POLLERR)) != 0)
			{
				readyKeys.push_back(keys[index]);
			}
		}
	}
	else
	{
		gLog.Error("Event loop stopped: WSAPoll failed: {}", DescribeError(WSAGetLastError()));
	}

	return isPolled;
}

//////////////////////////////////////////////////////////////////////////
// In chunks, so the number of watched sockets has no cap.
bool PollSockets(std::span<SPollerEntry const> entries, std::vector<uint64_t>& readyKeys)
{
	std::array<WSAPOLLFD, MaxSocketsPerPoll> polls{};
	std::array<uint64_t, MaxSocketsPerPoll> keys{};
	size_t numPolls{ 0 };
	bool isPolled{ true };

	for (SPollerEntry const& entry : entries)
	{
		if (isPolled && entry.waitable.kind == EWaitableKind::Socket)
		{
			// Writable too, though that alone never counts: asked only for reads, WSAPoll never reports a refused connect.
			polls[numPolls] = WSAPOLLFD{ static_cast<SOCKET>(entry.waitable.native), POLLRDNORM | POLLWRNORM, 0 };
			keys[numPolls] = entry.key;
			++numPolls;

			if (numPolls == polls.size())
			{
				isPolled = PollChunk(polls, keys, readyKeys);
				numPolls = 0;
			}
		}
	}

	if (isPolled && numPolls > 0)
	{
		isPolled = PollChunk(std::span{ polls }.first(numPolls), std::span{ keys }.first(numPolls), readyKeys);
	}

	return isPolled;
}

//////////////////////////////////////////////////////////////////////////
void CollectSignalledEvents(std::span<SPollerEntry const> entries, std::vector<uint64_t>& readyKeys)
{
	for (SPollerEntry const& entry : entries)
	{
		if (entry.waitable.kind == EWaitableKind::Event && WaitForSingleObject(reinterpret_cast<HANDLE>(entry.waitable.native), 0) == WAIT_OBJECT_0)
		{
			readyKeys.push_back(entry.key);
		}
	}
}
} // namespace

//////////////////////////////////////////////////////////////////////////
bool CPoller::Initialize()
{
	// The wake event clears itself in the wait that sees it; Wait clears the socket event before each poll.
	m_wakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	m_socketEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	bool const isReady{ m_wakeEvent != nullptr && m_socketEvent != nullptr };

	if (!isReady)
	{
		gLog.Error("Event loop: CreateEventW failed: {}", DescribeError(static_cast<int>(GetLastError())));
		Terminate();
	}

	return isReady;
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Terminate()
{
	if (m_socketEvent != nullptr && CloseHandle(m_socketEvent) == 0)
	{
		gLog.Warning("Event loop: cannot close the socket event: {}", DescribeError(static_cast<int>(GetLastError())));
	}

	if (m_wakeEvent != nullptr && CloseHandle(m_wakeEvent) == 0)
	{
		gLog.Warning("Event loop: cannot close the wake signal: {}", DescribeError(static_cast<int>(GetLastError())));
	}

	m_entries.clear();
	m_socketEvent = nullptr;
	m_wakeEvent = nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool CPoller::Add(SWaitable waitable, uint64_t key)
{
	bool const isWatched{ std::ranges::any_of(m_entries, [waitable](SPollerEntry const& entry) { return entry.waitable.native == waitable.native; }) };
	size_t const numEvents{ static_cast<size_t>(std::ranges::count(m_entries, EWaitableKind::Event, [](SPollerEntry const& entry) { return entry.waitable.kind; })) };
	bool isAdded{ false };

	if (isWatched)
	{
		gLog.Error("Event loop: cannot watch {} twice", waitable.native);
	}
	else if (waitable.kind == EWaitableKind::Socket)
	{
		isAdded = WSAEventSelect(static_cast<SOCKET>(waitable.native), m_socketEvent, SocketEvents) == 0;

		if (!isAdded)
		{
			gLog.Error("Event loop: cannot watch socket {}: {}", waitable.native, DescribeError(WSAGetLastError()));
		}
	}
	else if (numEvents < MaxEvents)
	{
		isAdded = true;
	}
	else
	{
		gLog.Error("Event loop: cannot watch more than {} events", MaxEvents);
	}

	if (isAdded)
	{
		m_entries.emplace_back(waitable, key);
	}

	return isAdded;
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Remove(SWaitable waitable)
{
	auto const found{ std::ranges::find(m_entries, waitable.native, [](SPollerEntry const& entry) { return entry.waitable.native; }) };

	if (found != m_entries.end())
	{
		if (found->waitable.kind == EWaitableKind::Socket && WSAEventSelect(static_cast<SOCKET>(waitable.native), nullptr, 0) != 0)
		{
			gLog.Warning("Event loop: cannot unwatch socket {}: {}", waitable.native, DescribeError(WSAGetLastError()));
		}

		m_entries.erase(found);
	}
	else
	{
		gLog.Warning("Event loop: cannot unwatch {}: it is not watched", waitable.native);
	}
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Wake()
{
	if (SetEvent(m_wakeEvent) == 0)
	{
		gLog.Error("Event loop: cannot wake the loop: {}", DescribeError(static_cast<int>(GetLastError())));
	}
}

//////////////////////////////////////////////////////////////////////////
bool CPoller::Wait(std::optional<std::chrono::milliseconds> timeout, std::vector<uint64_t>& readyKeys)
{
	readyKeys.clear();

	// Before the poll, so a socket that turns readable after it signals the event again.
	bool isWaiting{ ResetEvent(m_socketEvent) != 0 };

	if (isWaiting)
	{
		isWaiting = PollSockets(m_entries, readyKeys);
		CollectSignalledEvents(m_entries, readyKeys);
	}
	else
	{
		gLog.Error("Event loop stopped: cannot reset the socket event: {}", DescribeError(static_cast<int>(GetLastError())));
	}

	if (isWaiting && readyKeys.empty())
	{
		std::array<HANDLE, MAXIMUM_WAIT_OBJECTS> handles{ m_wakeEvent, m_socketEvent };
		std::array<uint64_t, MAXIMUM_WAIT_OBJECTS> keys{};
		DWORD numHandles{ NumOwnHandles };

		for (SPollerEntry const& entry : m_entries)
		{
			if (entry.waitable.kind == EWaitableKind::Event)
			{
				handles[numHandles] = reinterpret_cast<HANDLE>(entry.waitable.native);
				keys[numHandles] = entry.key;
				++numHandles;
			}
		}

		DWORD const signalled{ WaitForMultipleObjects(numHandles, handles.data(), FALSE, ToTimeoutMs(timeout)) };

		if (signalled == WAIT_OBJECT_0 + SocketEventIndex)
		{
			isWaiting = PollSockets(m_entries, readyKeys);
		}
		else if (signalled >= WAIT_OBJECT_0 + NumOwnHandles && signalled < WAIT_OBJECT_0 + numHandles)
		{
			// The wait has reset this event if it resets itself, so its key is reported now or never.
			readyKeys.push_back(keys[signalled - WAIT_OBJECT_0]);
		}
		else if (signalled == WAIT_FAILED)
		{
			gLog.Error("Event loop stopped: WaitForMultipleObjects failed: {}", DescribeError(static_cast<int>(GetLastError())));
			isWaiting = false;
		}
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
bool CPoller::Add(SWaitable, uint64_t)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CPoller::Remove(SWaitable)
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
