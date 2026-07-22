#include "module_graph.hpp"
#include <tge/logging/loggers.hpp>
#include <algorithm>
#include <limits>
#include <string>

namespace Tge
{
namespace
{
constexpr uint32_t Emitted{ std::numeric_limits<uint32_t>::max() };
constexpr size_t   NoModule{ std::numeric_limits<size_t>::max() };

//////////////////////////////////////////////////////////////////////////
IModule* FindLoaded(std::span<IModule* const> modules, IModuleId* pModuleId)
{
	auto const it = std::ranges::find_if(modules,
		[pModuleId](IModule* pModule) { return pModule->GetId() == pModuleId; });

	return it != modules.end() ? *it : nullptr;
}

//////////////////////////////////////////////////////////////////////////
// Notify edges are excluded on purpose: they carry no ordering, and counting them would restore the very
// cycles the kind split exists to break.
bool IsOrderingDependency(EDependencyKind kind)
{
	return kind == EDependencyKind::Required || kind == EDependencyKind::Optional;
}

//////////////////////////////////////////////////////////////////////////
bool DependsOn(IModule* pDependent, IModule* pDependency)
{
	bool depends{ false };

	for (SDependency const& dependency : pDependent->GetDependencies())
	{
		depends = depends
			|| (IsOrderingDependency(dependency.kind) && dependency.pModule == pDependency->GetId());
	}

	return depends;
}
} // namespace

//////////////////////////////////////////////////////////////////////////
bool CModuleGraph::Resolve(std::span<IModule* const> modules)
{
	m_order.clear();
	m_order.reserve(modules.size());

	bool resolved{ true };

	// Every offender is reported, so fixing a context takes one pass rather than one boot per mistake.
	for (auto* pModule : modules)
	{
		for (SDependency const& dependency : pModule->GetDependencies())
		{
			if (dependency.pModule == nullptr)
			{
				gLog.Error("Module '{}' declares a null dependency", pModule->GetName());

				resolved = false;
			}
			else if (dependency.kind == EDependencyKind::Required
			      && FindLoaded(modules, dependency.pModule) == nullptr)
			{
				gLog.Error("Module '{}' requires '{}', which this run context does not load",
					pModule->GetName(), dependency.pModule->GetName());

				resolved = false;
			}
		}
	}

	if (resolved)
	{
		// How many ordering dependencies each module is still waiting on; Emitted marks one already placed.
		std::vector<uint32_t> pending;
		pending.reserve(modules.size());

		for (auto* pModule : modules)
		{
			uint32_t waiting{ 0 };

			for (SDependency const& dependency : pModule->GetDependencies())
			{
				if (IsOrderingDependency(dependency.kind)
				 && FindLoaded(modules, dependency.pModule) != nullptr)
				{
					++waiting;
				}
			}

			pending.emplace_back(waiting);
		}

		while (resolved && m_order.size() < modules.size())
		{
			// Lowest input index among the ready modules, which is what makes the resolved order stable.
			size_t next{ NoModule };

			for (size_t index{ modules.size() }; index > 0; --index)
			{
				if (pending[index - 1] == 0)
				{
					next = index - 1;
				}
			}

			if (next == NoModule)
			{
				std::string cycle;

				for (size_t index{ 0 }; index < modules.size(); ++index)
				{
					if (pending[index] != Emitted)
					{
						cycle += modules[index]->GetName();
						cycle += " ";
					}
				}

				gLog.Error("Module dependency cycle among: {}- none of them can initialize first", cycle);

				resolved = false;
			}
			else
			{
				m_order.emplace_back(modules[next]);
				pending[next] = Emitted;

				for (size_t index{ 0 }; index < modules.size(); ++index)
				{
					if (pending[index] != Emitted && DependsOn(modules[index], modules[next]))
					{
						--pending[index];
					}
				}
			}
		}
	}

	return resolved;
}

//////////////////////////////////////////////////////////////////////////
std::span<IModule* const> CModuleGraph::GetInitializationOrder() const
{
	return m_order;
}
} // namespace Tge
