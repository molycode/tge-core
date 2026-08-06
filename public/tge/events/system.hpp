#pragma once

#include <tge/non_copyable.hpp>
#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace Tge::Events
{
using EventHandle = uint64_t;
constexpr EventHandle InvalidEventHandle = 0;

using EventTypeId = uint32_t;

namespace Impl
{
EventTypeId NextEventTypeId();
} // namespace Impl

// Sound only while every module links STATIC into one binary; a shared library gets its own counter.
template <typename TEvent>
EventTypeId EventTypeIdOf()
{
	static EventTypeId const id{ Impl::NextEventTypeId() };

	return id;
}

// Handlers run in subscription order on the EMITTING thread, so one touching scene/GPU state may only be
// emitted from the main thread.
class ISystem : public SNoCopyNoMove
{
public:

	// Subscribing mid-dispatch joins the next dispatch, not the one in flight. The handler may take
	// TEvent const& or nothing.
	template <typename TEvent, typename THandler>
	EventHandle Subscribe(THandler&& handler)
	{
		using Handler = std::remove_cvref_t<THandler>;

		return SubscribeErased(EventTypeIdOf<TEvent>(),
			[handler = std::forward<THandler>(handler)](void const* payload)
			{
				if constexpr (std::invocable<Handler const&, TEvent const&>)
				{
					handler(*static_cast<TEvent const*>(payload));
				}
				else
				{
					handler();
				}
			});
	}

	// Takes effect immediately, even mid-dispatch -- a handler that unsubscribes itself is tearing down.
	virtual void Unsubscribe(EventHandle handle) = 0;

	// Runs the handlers on THIS thread, before returning: a slow subscriber is the emitter's problem.
	template <typename TEvent>
	void Emit(TEvent const& payload)
	{
		EmitErased(EventTypeIdOf<TEvent>(), &payload);
	}

	template <typename TEvent>
	void Emit()
	{
		TEvent const payload{};

		EmitErased(EventTypeIdOf<TEvent>(), &payload);
	}

	// Pushes and returns without running a handler, so no subscriber can hold the emitter hostage. Callable
	// from any thread -- this is how a job-pool worker notifies the engine. The events module drains the
	// backlog on the main thread at the top of each frame, which is where the handler then runs.
	//
	// By value: the emitter's frame is gone by the time this dispatches, so a referenced payload dangles.
	template <typename TEvent>
	void QueueEmit(TEvent payload)
	{
		QueueErased([this, payload = std::move(payload)]()
		{
			EmitErased(EventTypeIdOf<TEvent>(), &payload);
		});
	}

	template <typename TEvent>
	void QueueEmit()
	{
		QueueErased([this]()
		{
			TEvent const payload{};

			EmitErased(EventTypeIdOf<TEvent>(), &payload);
		});
	}

protected:

	~ISystem() = default;

	virtual EventHandle SubscribeErased(EventTypeId type, std::function<void(void const*)> handler) = 0;
	virtual void EmitErased(EventTypeId type, void const* payload) = 0;
	virtual void QueueErased(std::function<void()> dispatch) = 0;
};

extern ISystem* gEvents;
} // namespace Tge::Events
