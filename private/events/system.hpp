#pragma once

#include <tge/events/system.hpp>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Tge::Events::Impl
{
struct SHandler final
{
	EventHandle                      handle{ InvalidEventHandle };
	std::function<void(void const*)> handler;

	// A dispatch walks a snapshot, so an entry outlives its subscription by one dispatch; this closes that window.
	std::atomic<bool> alive{ true };
};

using SHandlerList = std::vector<std::shared_ptr<SHandler>>;

class CSystem final : public ISystem
{
public:

	CSystem() = default;
	~CSystem() = default;

	bool Initialize();
	void Terminate();

	// Deliberately NOT on ISystem: draining anywhere but the module's main-thread Update would break the
	// guarantee that a queued handler runs on the main thread.
	void DispatchQueued();

	// Tge::Events::ISystem
	void Unsubscribe(EventHandle handle) override;
	// ~Tge::Events::ISystem

protected:

	// Tge::Events::ISystem
	EventHandle SubscribeErased(EventTypeId type, std::function<void(void const*)> handler) override;
	void EmitErased(EventTypeId type, void const* payload) override;
	void QueueErased(std::function<void()> dispatch) override;
	// ~Tge::Events::ISystem

private:

	struct SEventSlot final
	{
		SHandlerList                        handlers;
		std::shared_ptr<SHandlerList const> snapshot;
		size_t                              numDead{ 0 };
		bool                                dirty{ false };
	};

	struct SSubscription final
	{
		EventTypeId               type{ 0 };
		std::shared_ptr<SHandler> entry;
	};

	std::shared_ptr<SHandlerList const> GetSnapshot(EventTypeId type);

	// Dense so a dispatch indexes rather than hashes; the handle map serves only the cold mutation path.
	std::vector<SEventSlot>                        m_events;
	std::unordered_map<EventHandle, SSubscription> m_subscriptions;
	EventHandle                                    m_nextHandle{ 1 };
	std::mutex                                     m_mutex;

	// Its own lock: a worker queueing an event must never wait on a dispatch that is running handlers.
	std::vector<std::function<void()>> m_queue;
	std::mutex                         m_queueMutex;

	// Retained across drains so a steady stream of queued events stops reallocating.
	std::vector<std::function<void()>> m_draining;
};

extern CSystem gEvents;
} // namespace Tge::Events::Impl
