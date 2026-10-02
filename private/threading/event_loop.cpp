#include <tge/threading/event_loop.hpp>
#include "event_loop_state.hpp"
#include <tge/assert.hpp>
#include <tge/threading/job_system.hpp>
#include <tge/threading/thread_name.hpp>
#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

namespace Tge::Threading
{
namespace
{
thread_local CEventLoop const* tCurrentLoop{ nullptr };

//////////////////////////////////////////////////////////////////////////
std::vector<SEventLoopWatch>::iterator FindWatch(std::vector<SEventLoopWatch>& watches, uint64_t id)
{
	return std::ranges::find(watches, id, [](SEventLoopWatch const& entry) { return entry.id.value; });
}

//////////////////////////////////////////////////////////////////////////
bool IsSooner(SEventLoopTimer const& first, SEventLoopTimer const& second)
{
	return std::tie(first.when, first.id.value) < std::tie(second.when, second.id.value);
}
} // namespace

//////////////////////////////////////////////////////////////////////////
CEventLoop::CEventLoop() = default;

//////////////////////////////////////////////////////////////////////////
CEventLoop::~CEventLoop() = default;

//////////////////////////////////////////////////////////////////////////
bool CEventLoop::Initialize(std::string_view threadName)
{
	TGE_ASSERT(m_state == nullptr, "Event loop initialized twice");

	auto state = std::make_unique<SEventLoopState>();
	bool const isReady{ state->poller.Initialize() };

	if (isReady)
	{
		m_state = std::move(state);
		m_state->thread = std::thread{ &CEventLoop::Run, this, std::string{ threadName } };
	}

	return isReady;
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::Terminate()
{
	if (m_state != nullptr)
	{
		TGE_ASSERT(!IsLoopThread(), "Event loop terminated from its own thread");

		m_state->isStopping.store(true, std::memory_order_release);
		m_state->poller.Wake();
		m_state->thread.join();
		m_state->poller.Terminate();
		m_state.reset();
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::Post(std::function<void()> callback)
{
	TGE_ASSERT(m_state != nullptr, "Post to an event loop that is not running");
	TGE_ASSERT(callback != nullptr, "Post of an empty callback");

	m_state->posts.Enqueue(std::move(callback));
	m_state->poller.Wake();
}

//////////////////////////////////////////////////////////////////////////
std::optional<SWatchId> CEventLoop::Watch(int descriptor, std::function<void()> onReadable)
{
	TGE_ASSERT(IsLoopThread(), "Watch called off the event loop thread");
	TGE_ASSERT(onReadable != nullptr, "Watch with an empty callback");

	std::optional<SWatchId> watch{};
	SWatchId const id{ m_state->nextId };

	if (m_state->poller.Add(descriptor, id.value))
	{
		++m_state->nextId;
		m_state->watches.emplace_back(id, descriptor, std::move(onReadable));
		watch = id;
	}

	return watch;
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::Unwatch(SWatchId watch)
{
	TGE_ASSERT(IsLoopThread(), "Unwatch called off the event loop thread");

	auto const found{ FindWatch(m_state->watches, watch.value) };

	TGE_ASSERT(found != m_state->watches.end(), "Unwatch of a descriptor that is not watched");

	if (found != m_state->watches.end())
	{
		m_state->poller.Remove(found->descriptor);
		m_state->watches.erase(found);
	}
}

//////////////////////////////////////////////////////////////////////////
STimerId CEventLoop::ScheduleAt(std::chrono::steady_clock::time_point when, std::function<void()> callback)
{
	TGE_ASSERT(IsLoopThread(), "ScheduleAt called off the event loop thread");
	TGE_ASSERT(callback != nullptr, "ScheduleAt with an empty callback");

	STimerId const id{ m_state->nextId };

	++m_state->nextId;
	m_state->timers.emplace_back(id, when, std::move(callback));

	return id;
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::CancelTimer(STimerId timer)
{
	TGE_ASSERT(IsLoopThread(), "CancelTimer called off the event loop thread");

	std::erase_if(m_state->timers, [timer](SEventLoopTimer const& entry) { return entry.id.value == timer.value; });
}

//////////////////////////////////////////////////////////////////////////
bool CEventLoop::IsLoopThread() const
{
	return tCurrentLoop == this;
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::Run(std::string threadName)
{
	InitializeThread();
	SetCurrentThreadName(threadName);
	tCurrentLoop = this;

	bool isWaiting{ true };

	while (isWaiting && !m_state->isStopping.load(std::memory_order_acquire))
	{
		isWaiting = m_state->poller.Wait(GetTimeUntilNextTimer(), m_state->readyKeys);

		if (isWaiting)
		{
			RunReadyWatches();
			RunPosts();
			RunDueTimers();
		}
	}

	tCurrentLoop = nullptr;
	FinalizeThread();
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::RunReadyWatches()
{
	for (uint64_t const key : m_state->readyKeys)
	{
		auto const found{ FindWatch(m_state->watches, key) };

		// An earlier callback of this pass may have unwatched it.
		if (found != m_state->watches.end())
		{
			// A copy, since the callback may unwatch itself or add watches.
			std::function<void()> const onReadable{ found->onReadable };
			onReadable();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::RunPosts()
{
	std::function<void()> callback{};

	// Drained first, so a callback that posts again waits for the next pass instead of starving the descriptors.
	while (m_state->posts.Dequeue(callback))
	{
		m_state->runningPosts.emplace_back(std::move(callback));
	}

	for (std::function<void()> const& post : m_state->runningPosts)
	{
		post();
	}

	m_state->runningPosts.clear();
}

//////////////////////////////////////////////////////////////////////////
void CEventLoop::RunDueTimers()
{
	auto const now{ std::chrono::steady_clock::now() };
	// Timers these callbacks schedule wait for the next pass, even when already due.
	uint64_t const passEnd{ m_state->nextId };
	bool isDue{ true };

	while (isDue)
	{
		auto const next{ std::ranges::min_element(m_state->timers, IsSooner) };

		isDue = (next != m_state->timers.end()) && (next->when <= now) && (next->id.value < passEnd);

		if (isDue)
		{
			std::function<void()> const callback{ std::move(next->callback) };
			m_state->timers.erase(next);
			callback();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
std::optional<std::chrono::milliseconds> CEventLoop::GetTimeUntilNextTimer() const
{
	std::optional<std::chrono::milliseconds> timeout{};
	auto const next{ std::ranges::min_element(m_state->timers, IsSooner) };

	if (next != m_state->timers.end())
	{
		auto const remaining{ std::chrono::ceil<std::chrono::milliseconds>(next->when - std::chrono::steady_clock::now()) };
		timeout = std::max(remaining, std::chrono::milliseconds{ 0 });
	}

	return timeout;
}
} // namespace Tge::Threading
