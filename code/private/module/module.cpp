#include "module.hpp"
#include <tge/module/core_module.hpp>
#include <tge/init/init.hpp>

namespace Tge::Core
{
CModule gModuleImpl;
extern IModule* const gModule = static_cast<IModule*>(&gModuleImpl);

//////////////////////////////////////////////////////////////////////////
IModuleId* CModule::GetId() const
{
	return gModuleId;
}

//////////////////////////////////////////////////////////////////////////
bool CModule::Initialize()
{
	return Tge::Initialize();
}

//////////////////////////////////////////////////////////////////////////
void CModule::Terminate()
{
	Tge::Terminate();
}

//////////////////////////////////////////////////////////////////////////
void CModule::Update(EFramePhase phase, float deltaTime)
{
	Tge::Update();
}
} // namespace Tge::Core
