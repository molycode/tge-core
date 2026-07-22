#pragma once

#include <tge/module/frame_phase.hpp>
#include <tge/module/version.hpp>
#include <tge/non_copyable.hpp>
#include <cstdint>
#include <span>
#include <string_view>

namespace Tge
{
class IModuleId;

// Bump on any shape change to this contract - a module compiled against a different one cannot be driven safely.
constexpr uint32_t ModuleContractVersion{ 1 };

// Answering presence, preference and notification with one relation is what let the declared graph grow cycles.
enum class EDependencyKind : uint8_t
{
	Required, // must be present and initialized before me; absent means I cannot load
	Optional, // I use it when present and cope when absent; ordered before me when present
	Notify    // tell me when it initializes or terminates; not an ordering constraint
};

struct SDependency final
{
	IModuleId*      pModule{ nullptr };
	EDependencyKind kind{ EDependencyKind::Required };
	SVersion        minVersion{};
};

using Dependencies = std::span<SDependency const>;

class IModuleId : public SNoCopyNoMove
{
public:

	virtual std::string_view GetName() const = 0;
	virtual SVersion GetVersion() const = 0;
	virtual uint32_t GetContractVersion() const = 0;

protected:

	~IModuleId() = default;
};

class IModule : public SNoCopyNoMove
{
public:

	virtual IModuleId* GetId() const = 0;
	virtual Dependencies GetDependencies() const = 0;
	virtual bool Initialize() = 0;
	virtual void Terminate() = 0;
	// Called once per phase the module is scheduled in, so one spanning phases must branch on `phase`.
	// Defaulted for the modules that declare no tick at all; the runtime rejects a module that is neither
	// scheduled nor declared tickless, so leaving this alone cannot silently cost a module its frame.
	virtual void Update(EFramePhase phase, float deltaTime) {}
	virtual void OnDependencyInitialized(IModuleId* pDependency) = 0;
	virtual void OnDependencyTerminating(IModuleId* pDependency) = 0;

	std::string_view GetName() const { return GetId()->GetName(); }

protected:

	~IModule() = default;
};

namespace Core
{
// Identity only -- Core's drivable gModule stays in <tge/module/core_module.hpp>, out of this everything-includes header.
extern IModuleId* const gModuleId;
} // namespace Core
} // namespace Tge
