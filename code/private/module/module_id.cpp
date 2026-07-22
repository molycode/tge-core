#include <tge/module/module_id.hpp>

namespace Tge::Core
{
CModuleId gModuleIdImpl{ "Core" };
extern IModuleId* const gModuleId = static_cast<IModuleId*>(&gModuleIdImpl);
} // namespace Tge::Core
