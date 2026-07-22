#pragma once

#include <tge/module/module.hpp>
#include <tge/non_copyable.hpp>
#include <span>
#include <vector>

namespace Tge
{
class CModuleGraph final : public SNoCopyNoMove
{
public:

	CModuleGraph() = default;
	~CModuleGraph() = default;

	// Orders `modules` so every Required and Optional dependency precedes its dependent. Ties keep the input
	// order, so a context author cannot change the outcome by listing an unconstrained module elsewhere.
	bool Resolve(std::span<IModule* const> modules);

	std::span<IModule* const> GetInitializationOrder() const;

private:

	std::vector<IModule*> m_order;
};
} // namespace Tge
