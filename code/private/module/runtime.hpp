#pragma once

#include "module_graph.hpp"
#include <tge/module/runtime.hpp>
#include <tge/module/frame_phase.hpp>
#include <array>
#include <vector>

namespace Tge
{
class IModule;
struct SRunContext;

class CRuntime final : public IRuntime
{
public:

	CRuntime() = default;
	~CRuntime() = default;

	bool Initialize(SRunContext const& context) override;
	void Terminate() override;
	void Update(float deltaTime) override;

	void Quit() override;
	bool CanRun() const override;

private:

	void RegisterModule(IModule* pModule);
	void UnregisterModule(IModule* pModule);
	void RegisterUpdate(EFramePhase phase, IModule* pModule);
	void DeclareNoTick(IModule* pModule);
	void ValidateSchedule() const;
	void LogFrameSchedule() const;
	void LogModuleVersions() const;
	bool InitializeModules();
	void NotifyDependentsInitialized(IModule* pModule);
	void TerminateModule(IModule* pModule);
	void TerminateModules();

	std::vector<IModule*> m_modules;
	std::vector<IModule*> m_activeModules;

	// Initialization order, derived from the declared dependencies rather than from registration order.
	CModuleGraph m_moduleGraph;

	// Only what appears here ticks — registration alone does not schedule a module.
	std::array<std::vector<IModule*>, FramePhaseCount> m_schedule;

	// Modules that deliberately never tick. Being absent from both this and the schedule is the mistake
	// ValidateSchedule exists to catch, so silence has to be declared rather than assumed.
	std::vector<IModule*> m_noTick;

	bool m_running = true;
};

extern CRuntime gRuntimeImpl;
} // namespace Tge
