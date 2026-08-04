#include <tge/events/module_id.hpp>
#include <tge/module/module_id.hpp>
#include <tge/module/version.hpp>

namespace Tge::Events
{
CModuleId gModuleIdImpl{ "Events", SVersion{ 1, 0, 0 } };
extern IModuleId* const gModuleId = static_cast<IModuleId*>(&gModuleIdImpl);
} // namespace Tge::Events
