#include "system.hpp"
#include <utility>

namespace Tge::Events
{
namespace Impl
{
CSystem gEvents;

//////////////////////////////////////////////////////////////////////////
bool CSystem::Initialize()
{
	Events::gEvents = this;

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Terminate()
{
	Events::gEvents = nullptr;
}

//////////////////////////////////////////////////////////////////////////
EventHandle CSystem::SubscribeErased(EventTypeId const type, std::function<void(void const*)> handler)
{
	std::lock_guard<std::mutex> const lock(m_mutex);

	EventHandle const handle{ m_nextHandle++ };

	auto entry{ std::make_shared<SHandler>() };
	entry->handle  = handle;
	entry->handler = std::move(handler);

	if (type >= m_events.size())
	{
		m_events.resize(static_cast<size_t>(type) + 1u);
	}

	SEventSlot& slot = m_events[type];
	slot.handlers.emplace_back(entry);

	// Marked, not rebuilt -- a rebuild per mutation would make a burst of N subscribes O(N^2).
	slot.dirty = true;

	m_subscriptions[handle] = SSubscription{ type, std::move(entry) };

	return handle;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::Unsubscribe(EventHandle handle)
{
	std::lock_guard<std::mutex> const lock(m_mutex);

	auto const subscriptionIt = m_subscriptions.find(handle);

	if (subscriptionIt != m_subscriptions.end())
	{
		SEventSlot& slot = m_events[subscriptionIt->second.type];

		subscriptionIt->second.entry->alive.store(false, std::memory_order_relaxed);
		++slot.numDead;
		slot.dirty = true;

		// Compacted here only when the dead outnumber the living: an event that churns but rarely emits would
		// otherwise never reach the rebuild that drops them.
		if ((slot.numDead * 2u) > slot.handlers.size())
		{
			std::erase_if(slot.handlers, [](std::shared_ptr<SHandler> const& entry)
			{
				return !entry->alive.load(std::memory_order_relaxed);
			});

			slot.numDead = 0u;
		}

		m_subscriptions.erase(subscriptionIt);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::EmitErased(EventTypeId const type, void const* payload)
{
	std::shared_ptr<SHandlerList const> const snapshot{ GetSnapshot(type) };

	// No lock held -- holding one serializes emitters behind each other's handlers and deadlocks a re-entrant one.
	if (snapshot != nullptr)
	{
		for (std::shared_ptr<SHandler> const& entry : *snapshot)
		{
			if (entry->alive.load(std::memory_order_relaxed))
			{
				entry->handler(payload);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSystem::QueueErased(std::function<void()> dispatch)
{
	std::lock_guard<std::mutex> const lock(m_queueMutex);

	m_queue.emplace_back(std::move(dispatch));
}

//////////////////////////////////////////////////////////////////////////
void CSystem::DispatchQueued()
{
	{
		std::lock_guard<std::mutex> const lock(m_queueMutex);

		m_queue.swap(m_draining);
	}

	// Swapped out before dispatching, so an event a handler queues lands in the NEXT drain rather than
	// extending this one into an unbounded loop.
	for (std::function<void()> const& dispatch : m_draining)
	{
		dispatch();
	}

	m_draining.clear();
}

//////////////////////////////////////////////////////////////////////////
std::shared_ptr<SHandlerList const> CSystem::GetSnapshot(EventTypeId const type)
{
	std::lock_guard<std::mutex> const lock(m_mutex);

	std::shared_ptr<SHandlerList const> snapshot;

	// Past the end means nobody ever subscribed to this type.
	if (type < m_events.size())
	{
		SEventSlot& slot = m_events[type];

		if (slot.dirty)
		{
			// The rebuild already copies, so it is the cheapest place to drop what Unsubscribe only marked.
			std::erase_if(slot.handlers, [](std::shared_ptr<SHandler> const& entry)
			{
				return !entry->alive.load(std::memory_order_relaxed);
			});

			slot.numDead  = 0u;
			slot.snapshot = std::make_shared<SHandlerList const>(slot.handlers);
			slot.dirty    = false;
		}

		snapshot = slot.snapshot;
	}

	return snapshot;
}
} // namespace Impl
} // namespace Tge::Events
