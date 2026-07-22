#include <tge/module/module_id.hpp>
#include <tge/module/version.hpp>

namespace Tge::Core
{
CModuleId gModuleIdImpl{ "Core", SVersion{ 1, 0, 0 } };
extern IModuleId* const gModuleId = static_cast<IModuleId*>(&gModuleIdImpl);
} // namespace Tge::Core
