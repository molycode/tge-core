#pragma once

#include <tge/module/module.hpp>

namespace Tge::Core
{
class CModule final : public IModule
{
public:

	CModule() = default;
	~CModule() = default;

	// Tge::IModule
	IModuleId* GetId() const override;
	Tge::Dependencies GetDependencies() const override { return {}; }
	bool Initialize() override;
	void Terminate() override;
	void Update(EFramePhase phase, float deltaTime) override;
	void OnDependencyInitialized(IModuleId* pDependency) override {}
	void OnDependencyTerminating(IModuleId* pDependency) override {}
	// ~Tge::IModule
};

extern CModule gModuleImpl;
} // namespace Tge::Core
