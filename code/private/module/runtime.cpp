#include "runtime.hpp"
#include <tge/logging/loggers.hpp>
#include <tge/logging/log_system.hpp>
#include <tge/module/module.hpp>
#include <tge/module/run_context.hpp>
#include <tge/profiling/profiling.hpp>
#include <algorithm>
#include <chrono>
#include <format>
#include <string>

namespace Tge
{
namespace
{
//////////////////////////////////////////////////////////////////////////
bool Declares(IModule* pDependent, IModule* pModule)
{
	return std::ranges::any_of(pDependent->GetDependencies(),
		[pModule](SDependency const& dependency) { return dependency.pModule == pModule->GetId(); });
}
} // namespace

CRuntime gRuntimeImpl;
extern IRuntime* const gRuntime = static_cast<IRuntime*>(&gRuntimeImpl);

//////////////////////////////////////////////////////////////////////////
bool CRuntime::Initialize(SRunContext const& context)
{
	// Initialize logging system first - must happen before any logging
	Logging::GetLogSystem().Initialize();

	bool initialized{ false };

	for (auto* pModule : context.modules)
	{
		RegisterModule(pModule);
	}

	for (SScheduleEntry const& entry : context.schedule)
	{
		RegisterUpdate(entry.phase, entry.pModule);
	}

	for (auto* pModule : context.noTick)
	{
		DeclareNoTick(pModule);
	}

	// A context that cannot tick correctly should not get as far as a Vulkan device.
	ValidateSchedule();
	LogFrameSchedule();
	LogModuleVersions();

	if (InitializeModules())
	{
		initialized = true;
	}
	else
	{
		gLog.Error("Failed to initialize modules");
	}

	return initialized;
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::Terminate()
{
	TerminateModules();

	// Terminate logging system last
	Logging::GetLogSystem().Terminate();
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::Update(float deltaTime)
{
	TGE_PROFILE_SCOPE_N("Runtime: update");

	// Clamped here, once, so every module advances by the same bounded timestep — the whole point of a single
	// authoritative delta is that there is one place to do this.
	float const frameDelta{ std::min(deltaTime, MaxFrameDelta) };

	// Decorated rather than reusing GetFramePhaseName so a plot cannot collide with a same-named zone --
	// "Pace" is both a phase and a renderer zone.
	static constexpr std::array PhasePlotNames
	{
		"Phase FrameStart (ms)",
		"Phase Pace (ms)",
		"Phase Input (ms)",
		"Phase Simulate (ms)",
		"Phase Render (ms)",
		"Phase PostRender (ms)"
	};

	static_assert(PhasePlotNames.size() == FramePhaseCount, "a frame phase is missing its plot name");

	for (size_t index{ 0 }; index < FramePhaseCount; ++index)
	{
		EFramePhase const phase = static_cast<EFramePhase>(index);

		// Phase totals span every module in the phase, so no module-internal zone can report them.
		TGE_PROFILE_PLOT_SCOPE_MS(PhasePlotNames[index]);

		for (auto* pModule : m_schedule[index])
		{
			pModule->Update(phase, frameDelta);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::Quit()
{
	m_running = false;
}

//////////////////////////////////////////////////////////////////////////
bool CRuntime::CanRun() const
{
	return m_running;
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::RegisterModule(IModule* pModule)
{
	if (std::ranges::find(m_modules, pModule) == m_modules.end())
	{
		m_modules.emplace_back(pModule);
	}
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::UnregisterModule(IModule* pModule)
{
	auto it = std::ranges::find(m_modules, pModule);

	if (it != m_modules.end())
	{
		m_modules.erase(it);
	}
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::RegisterUpdate(EFramePhase phase, IModule* pModule)
{
	if (std::ranges::find(m_modules, pModule) != m_modules.end())
	{
		m_schedule[static_cast<size_t>(phase)].emplace_back(pModule);
	}
	else
	{
		gLog.Error("Module '{}' is scheduled in phase {} but was never registered - it will never tick",
			pModule->GetName(), GetFramePhaseName(phase));
	}
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::DeclareNoTick(IModule* pModule)
{
	m_noTick.emplace_back(pModule);
}

//////////////////////////////////////////////////////////////////////////
// Scheduling is opt-in, so forgetting it costs a module its whole per-frame existence with no other symptom.
void CRuntime::ValidateSchedule() const
{
	for (auto* pModule : m_modules)
	{
		bool scheduled{ std::ranges::find(m_noTick, pModule) != m_noTick.end() };

		for (auto const& phaseModules : m_schedule)
		{
			scheduled = scheduled || (std::ranges::find(phaseModules, pModule) != phaseModules.end());
		}

		if (!scheduled)
		{
			gLog.Error("Module '{}' is registered but never ticks - add a RegisterUpdate for it, or "
			           "DeclareNoTick if that is intended", pModule->GetName());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// A module missing from the schedule silently never ticks; this line is where that is visible.
void CRuntime::LogFrameSchedule() const
{
	std::string schedule;

	for (size_t index{ 0 }; index < FramePhaseCount; ++index)
	{
		schedule += GetFramePhaseName(static_cast<EFramePhase>(index));
		schedule += "[";

		for (auto* pModule : m_schedule[index])
		{
			schedule += pModule->GetName();
			schedule += " ";
		}

		schedule += "] ";
	}

	gLog.Info("Frame schedule: {}", schedule);
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::LogModuleVersions() const
{
	std::string versions;

	for (auto* pModule : m_modules)
	{
		versions += std::format("{} {} ", pModule->GetName(), pModule->GetId()->GetVersion());
	}

	gLog.Info("Module versions: {}", versions);
}

//////////////////////////////////////////////////////////////////////////
bool CRuntime::InitializeModules()
{
	bool initialized{ m_moduleGraph.Resolve(m_modules) };

	if (initialized)
	{
		auto const totalStartTime = std::chrono::high_resolution_clock::now();
		auto const order          = m_moduleGraph.GetInitializationOrder();

		for (size_t index{ 0 }; index < order.size() && initialized; ++index)
		{
			IModule* pModule = order[index];

			gLog.Info("Initializing {}...", pModule->GetName());

			auto const moduleStartTime = std::chrono::high_resolution_clock::now();

			if (pModule->Initialize())
			{
				auto const moduleEndTime = std::chrono::high_resolution_clock::now();
				auto const duration = std::chrono::duration_cast<std::chrono::microseconds>(moduleEndTime - moduleStartTime);
				double const milliseconds{ duration.count() / 1000.0 };

				m_activeModules.emplace_back(pModule);

				gLog.Info("Initialized {} in {:.2f} ms", pModule->GetName(), milliseconds);

				NotifyDependentsInitialized(pModule);
			}
			else
			{
				gLog.Error("{} failed to initialize", pModule->GetName());
				TerminateModule(pModule);

				initialized = false;
			}
		}

		if (initialized)
		{
			auto const totalEndTime = std::chrono::high_resolution_clock::now();
			auto const totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(totalEndTime - totalStartTime);
			double const totalMilliseconds{ totalDuration.count() / 1000.0 };

			gLog.Info("All modules initialized in {:.2f} ms", totalMilliseconds);
		}
	}

	return initialized;
}

//////////////////////////////////////////////////////////////////////////
// Every kind notifies. Notify means "notification without ordering", not "the only kind notified".
void CRuntime::NotifyDependentsInitialized(IModule* pModule)
{
	for (auto* pDependent : m_modules)
	{
		if (Declares(pDependent, pModule))
		{
			pDependent->OnDependencyInitialized(pModule->GetId());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// The "Terminating" line is the only thing that names the module if Terminate hangs or aborts.
void CRuntime::TerminateModule(IModule* pModule)
{
	gLog.Info("Terminating {}...", pModule->GetName());

	auto const startTime = std::chrono::high_resolution_clock::now();

	pModule->Terminate();

	auto const endTime = std::chrono::high_resolution_clock::now();
	auto const duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
	double const milliseconds{ duration.count() / 1000.0 };

	gLog.Info("Terminated {} in {:.2f} ms", pModule->GetName(), milliseconds);
}

//////////////////////////////////////////////////////////////////////////
void CRuntime::TerminateModules()
{
	bool const hadActiveModules{ !m_activeModules.empty() };
	auto const totalStartTime = std::chrono::high_resolution_clock::now();

	while (!m_activeModules.empty())
	{
		IModule* pModuleToTerminate = m_activeModules.back();

		for (auto* pDependent : m_activeModules)
		{
			if (pDependent != pModuleToTerminate && Declares(pDependent, pModuleToTerminate))
			{
				pDependent->OnDependencyTerminating(pModuleToTerminate->GetId());
			}
		}

		TerminateModule(pModuleToTerminate);
		m_activeModules.pop_back();
	}

	if (hadActiveModules)
	{
		auto const totalEndTime = std::chrono::high_resolution_clock::now();
		auto const totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(totalEndTime - totalStartTime);
		double const totalMilliseconds{ totalDuration.count() / 1000.0 };

		gLog.Info("All modules terminated in {:.2f} ms", totalMilliseconds);
	}

	m_modules.clear();
}
} // namespace Tge
