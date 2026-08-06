#include "module.hpp"
#include "system.hpp"
#include <tge/events/module.hpp>
#include <tge/events/module_id.hpp>

namespace Tge::Events
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
	return Impl::gEvents.Initialize();
}

//////////////////////////////////////////////////////////////////////////
void CModule::Terminate()
{
	Impl::gEvents.Terminate();
}

//////////////////////////////////////////////////////////////////////////
void CModule::Update(EFramePhase phase, float deltaTime)
{
	Impl::gEvents.DispatchQueued();
}
} // namespace Tge::Events
