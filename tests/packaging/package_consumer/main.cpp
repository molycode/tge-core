#include <tge/config.hpp>
#include <tge/events/module.hpp>
#include <tge/events/system.hpp>
#include <tge/logging/log.hpp>
#include <tge/math/functions.hpp>
#include <tge/math/transform.hpp>
#include <tge/module/core_module.hpp>
#include <tge/module/module.hpp>
#include <tge/module/run_context.hpp>
#include <tge/module/runtime.hpp>
#include <tge/module/version.hpp>
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace
{
using namespace Tge;

Logging::CLog gLog{ "Consumer" };

IModule* const Modules[]
{
	Core::gModule,
	Events::gModule
};

// Events ahead of Core in the phase, because its Update is what drains the backlog the check below queues into.
SScheduleEntry const Schedule[]
{
	{ EFramePhase::FrameStart, Events::gModule },
	{ EFramePhase::FrameStart, Core::gModule }
};

SRunContext const Context{ Modules, Schedule, {} };

constexpr float TickDelta{ 1.0f / 60.0f };

std::string_view const ExpectedName{ "Core" };

// Declared here rather than taken from a module's header: core sits below every module and may not name one
// of their events, so neither may its gate. The unit suite declares its own for the same reason.
struct SPackagedEvent final
{
	uint32_t value{ 0 };
};

constexpr uint32_t ExpectedPayload{ 0xC0DEu };

// Reaching gModuleId at all proves the archive linked; the values prove the identity survived packaging.
bool VerifyCoreIdentity()
{
	bool verified{ false };

	SVersion const expectedVersion{ 1, 0, 0 };

	if (Core::gModuleId != nullptr)
	{
		std::string_view const name{ Core::gModuleId->GetName() };
		SVersion const         version{ Core::gModuleId->GetVersion() };
		uint32_t const         contract{ Core::gModuleId->GetContractVersion() };

		bool const identified{ (name == ExpectedName) && Satisfies(version, expectedVersion) };
		bool const contractAgrees{ (contract == ModuleContractVersion)
		                        && (contract == TGE_PACKAGE_CONTRACT_VERSION) };

		if (identified && contractAgrees)
		{
			gLog.Info("Core package reports {} {}, module contract {}", name, version, contract);

			verified = true;
		}
		else if (!identified)
		{
			gLog.Error("Core package reports {} {}, expected {} {}",
			           name, version, ExpectedName, expectedVersion);
		}
		else
		{
			gLog.Error("Core package binaries speak module contract {}, but its config publishes {} and its "
			           "header {}", contract, TGE_PACKAGE_CONTRACT_VERSION, ModuleContractVersion);
		}
	}
	else
	{
		gLog.Error("Core package published no module id");
	}

	return verified;
}

// gEvents is published by the dispatcher in libTgeEvents.a while a module compiles against the interface
// alone, so a queued event coming back is a link-time statement that the implementation -- not merely the
// module's identity -- satisfied this binary. Queued rather than emitted, because only the frame phase
// drains it: an install shipping the interface without the module that ticks it would hang here at zero.
bool VerifyQueuedEventDispatches()
{
	bool verified{ false };

	if (Events::gEvents != nullptr)
	{
		uint32_t received{ 0 };

		Events::EventHandle const handle{ Events::gEvents->Subscribe<SPackagedEvent>(
			[&received](SPackagedEvent const& event) { received = event.value; }) };

		Events::gEvents->QueueEmit(SPackagedEvent{ ExpectedPayload });

		bool const heldUntilTheFrame{ received == 0 };

		gRuntime->Update(TickDelta);

		Events::gEvents->Unsubscribe(handle);

		if (heldUntilTheFrame && (received == ExpectedPayload))
		{
			gLog.Info("Core package dispatches a queued event on the frame phase that drains it");

			verified = true;
		}
		else if (!heldUntilTheFrame)
		{
			gLog.Error("QueueEmit dispatched before the frame phase ran");
		}
		else
		{
			gLog.Error("Queued event delivered {:#x}, expected {:#x}", received, ExpectedPayload);
		}
	}
	else
	{
		gLog.Error("Core package published no event system");
	}

	return verified;
}

// glm reaches a consumer only through the copy this package installs, and CTransform::FromMatrix lives in
// libTgeMath.a: the include is the header assertion, the round trip the link one.
bool VerifyMathRoundTrip()
{
	Math::CTransform transform;
	transform.position = Math::Vec3{ 1.0f, 2.0f, 3.0f };

	Math::CTransform const restored{ Math::CTransform::FromMatrix(transform.ToMatrix()) };

	bool const roundTripped{ Math::Distance(restored.position, transform.position) < 0.001f };

	if (roundTripped)
	{
		gLog.Info("Core package round-trips a transform through glm's headers and libTgeMath.a");
	}
	else
	{
		gLog.Error("Transform round trip returned ({}, {}, {})",
		           restored.position.x, restored.position.y, restored.position.z);
	}

	return roundTripped;
}

// TGE_BUILD_TYPE is defined by the generated tge/config.hpp and by nothing else -- no target carries it on
// the command line -- so naming it is what says the flag header itself travelled with the package.
bool VerifyGeneratedConfigShipped()
{
	std::string_view const buildType{ TGE_BUILD_TYPE };
	bool const             shipped{ !buildType.empty() };

	if (shipped)
	{
		gLog.Info("Core package ships its generated config header, built as {}", buildType);
	}
	else
	{
		gLog.Error("Core package shipped a config header naming no build type");
	}

	return shipped;
}
} // namespace

int main()
{
	int result{ EXIT_FAILURE };

	if (gRuntime->Initialize(Context))
	{
		bool const identityVerified{ VerifyCoreIdentity() };
		bool const eventsVerified{ VerifyQueuedEventDispatches() };
		bool const mathVerified{ VerifyMathRoundTrip() };
		bool const configVerified{ VerifyGeneratedConfigShipped() };

		// Ahead of Terminate, which takes the log system down with it and would leave the gate's marker
		// coming out of the pre-init fallback path.
		if (identityVerified && eventsVerified && mathVerified && configVerified)
		{
			gLog.Info("Consumer linked the packaged Core implementation from packages alone");

			result = EXIT_SUCCESS;
		}

		gRuntime->Terminate();
	}
	else
	{
		gLog.Error("Runtime initialization failed");
	}

	return result;
}
