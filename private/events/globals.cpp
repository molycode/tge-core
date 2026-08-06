#include <tge/events/system.hpp>
#include <atomic>

namespace Tge::Events
{
ISystem* gEvents{ nullptr };

namespace Impl
{
//////////////////////////////////////////////////////////////////////////
EventTypeId NextEventTypeId()
{
	// Two threads first touching two DIFFERENT event types race here: the standard serialises each
	// function-local static's own initialization, not two separate ones calling in.
	static std::atomic<EventTypeId> next{ 0 };

	return next.fetch_add(1, std::memory_order_relaxed);
}
} // namespace Impl
} // namespace Tge::Events
