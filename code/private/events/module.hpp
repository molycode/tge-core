#pragma once

#include <tge/module/module.hpp>

namespace Tge::Events
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

	// Registered first, so the queued backlog drains at the top of the frame, before any module reads it.
	void Update(EFramePhase phase, float deltaTime) override;
	void OnDependencyInitialized(IModuleId* pDependency) override {}
	void OnDependencyTerminating(IModuleId* pDependency) override {}
	// ~Tge::IModule
};

extern CModule gModuleImpl;
} // namespace Tge::Events
