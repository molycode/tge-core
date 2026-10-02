#pragma once

#include <tge/module/module.hpp>
#include <tge/threading/job_system.hpp>
#include <cstddef>

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

	void SetNumWorkerThreads(size_t numWorkerThreads);

private:

	size_t m_numWorkerThreads{ Threading::AutoThreadCount };
};

extern CModule gModuleImpl;
} // namespace Tge::Core
