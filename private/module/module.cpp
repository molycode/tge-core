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
	return Tge::Initialize(m_numWorkerThreads);
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

//////////////////////////////////////////////////////////////////////////
void CModule::SetNumWorkerThreads(size_t numWorkerThreads)
{
	m_numWorkerThreads = numWorkerThreads;
}
} // namespace Tge::Core
