#include "module_graph.hpp"
#include <tge/module/module_id.hpp>
#include <tge/module/version.hpp>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

namespace
{
using Tge::CModuleGraph;
using Tge::CModuleId;
using Tge::Dependencies;
using Tge::EDependencyKind;
using Tge::IModuleId;
using Tge::SDependency;
using Tge::SVersion;

// Exercises the sort through the interface the real modules implement, not a reshaped copy of it.
class CFakeModule final : public Tge::IModule
{
public:

	// A real module bakes the contract version it compiled against, so a mismatch is only constructible here.
	explicit CFakeModule(std::string_view name, SVersion version = SVersion{ 1, 0, 0 },
	                     uint32_t contractVersion = Tge::ModuleContractVersion)
		: m_id{ name, version, contractVersion }
	{}
	~CFakeModule() = default;

	void DependsOn(std::vector<SDependency> dependencies) { m_dependencies = std::move(dependencies); }

	// Tge::IModule
	IModuleId* GetId() const override { return const_cast<CModuleId*>(&m_id); }
	Dependencies GetDependencies() const override { return m_dependencies; }
	bool Initialize() override { return true; }
	void Terminate() override {}
	void OnDependencyInitialized(IModuleId* pDependency) override {}
	void OnDependencyTerminating(IModuleId* pDependency) override {}
	// ~Tge::IModule

private:

	CModuleId                m_id;
	std::vector<SDependency> m_dependencies;
};

//////////////////////////////////////////////////////////////////////////
std::vector<std::string_view> NamesOf(std::span<Tge::IModule* const> modules)
{
	std::vector<std::string_view> names;

	for (auto* pModule : modules)
	{
		names.emplace_back(pModule->GetName());
	}

	return names;
}
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, OrdersADependencyBeforeItsDependent)
{
	CFakeModule a{ "A" };
	CFakeModule b{ "B" };
	CFakeModule c{ "C" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Required } });
	c.DependsOn({ { b.GetId(), EDependencyKind::Required } });

	// Deliberately the reverse of the answer, so passing cannot mean "the input was already sorted".
	Tge::IModule* modules[] = { &c, &b, &a };

	CModuleGraph graph;

	ASSERT_TRUE(graph.Resolve(modules));
	EXPECT_EQ(NamesOf(graph.GetInitializationOrder()), (std::vector<std::string_view>{ "A", "B", "C" }));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, OrdersAnOptionalDependencyBeforeItsDependentWhenPresent)
{
	CFakeModule a{ "A" };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Optional } });

	Tge::IModule* modules[] = { &b, &a };

	CModuleGraph graph;

	ASSERT_TRUE(graph.Resolve(modules));
	EXPECT_EQ(NamesOf(graph.GetInitializationOrder()), (std::vector<std::string_view>{ "A", "B" }));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, ResolvesWithAnOptionalDependencyAbsent)
{
	CFakeModule a{ "A" };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Optional } });

	Tge::IModule* modules[] = { &b };

	CModuleGraph graph;

	ASSERT_TRUE(graph.Resolve(modules));
	EXPECT_EQ(NamesOf(graph.GetInitializationOrder()), (std::vector<std::string_view>{ "B" }));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, FailsWithARequiredDependencyAbsent)
{
	CFakeModule a{ "A" };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Required } });

	Tge::IModule* modules[] = { &b };

	CModuleGraph graph;

	EXPECT_FALSE(graph.Resolve(modules));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, FailsOnACycle)
{
	CFakeModule a{ "A" };
	CFakeModule b{ "B" };

	a.DependsOn({ { b.GetId(), EDependencyKind::Required } });
	b.DependsOn({ { a.GetId(), EDependencyKind::Required } });

	Tge::IModule* modules[] = { &a, &b };

	CModuleGraph graph;

	EXPECT_FALSE(graph.Resolve(modules));
}

//////////////////////////////////////////////////////////////////////////
// The relation that broke the real graph: Renderer and Debug notify each other and must still order.
TEST(ModuleGraph, ResolvesMutualNotifyEdges)
{
	CFakeModule a{ "A" };
	CFakeModule b{ "B" };

	a.DependsOn({ { b.GetId(), EDependencyKind::Notify } });
	b.DependsOn({ { a.GetId(), EDependencyKind::Notify } });

	Tge::IModule* modules[] = { &a, &b };

	CModuleGraph graph;

	ASSERT_TRUE(graph.Resolve(modules));
	EXPECT_EQ(NamesOf(graph.GetInitializationOrder()), (std::vector<std::string_view>{ "A", "B" }));
}

//////////////////////////////////////////////////////////////////////////
// Unconstrained modules keep the input order, which is what lets a context list its modules freely without
// the resolved order shifting under it.
TEST(ModuleGraph, KeepsTheInputOrderOfIndependentModules)
{
	CFakeModule a{ "A" };
	CFakeModule b{ "B" };
	CFakeModule c{ "C" };

	Tge::IModule* modules[] = { &c, &a, &b };

	CModuleGraph graph;

	ASSERT_TRUE(graph.Resolve(modules));
	EXPECT_EQ(NamesOf(graph.GetInitializationOrder()), (std::vector<std::string_view>{ "C", "A", "B" }));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, ResolvesWithADependencyAboveTheDeclaredMinimum)
{
	CFakeModule a{ "A", SVersion{ 4, 7, 2 } };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Required, SVersion{ 4, 1, 0 } } });

	Tge::IModule* modules[] = { &b, &a };

	CModuleGraph graph;

	EXPECT_TRUE(graph.Resolve(modules));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, FailsWithADependencyBelowTheDeclaredMinimum)
{
	CFakeModule a{ "A", SVersion{ 4, 0, 9 } };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Required, SVersion{ 4, 1, 0 } } });

	Tge::IModule* modules[] = { &b, &a };

	CModuleGraph graph;

	EXPECT_FALSE(graph.Resolve(modules));
}

//////////////////////////////////////////////////////////////////////////
// A major bump is a breaking interface change, so satisfying the minimum is not enough on its own.
TEST(ModuleGraph, FailsWithADependencyOfANewerMajor)
{
	CFakeModule a{ "A", SVersion{ 5, 0, 0 } };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Required, SVersion{ 4, 1, 0 } } });

	Tge::IModule* modules[] = { &b, &a };

	CModuleGraph graph;

	EXPECT_FALSE(graph.Resolve(modules));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, ResolvesWithAnUnsatisfiableConstraintOnAnAbsentOptionalDependency)
{
	CFakeModule a{ "A", SVersion{ 1, 0, 0 } };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Optional, SVersion{ 9, 0, 0 } } });

	Tge::IModule* modules[] = { &b };

	CModuleGraph graph;

	EXPECT_TRUE(graph.Resolve(modules));
}

//////////////////////////////////////////////////////////////////////////
// A Notify dependency carries no ordering, but it is still an interface the dependent compiles against.
TEST(ModuleGraph, FailsWithANotifyDependencyBelowTheDeclaredMinimum)
{
	CFakeModule a{ "A", SVersion{ 1, 0, 0 } };
	CFakeModule b{ "B" };

	b.DependsOn({ { a.GetId(), EDependencyKind::Notify, SVersion{ 2, 0, 0 } } });

	Tge::IModule* modules[] = { &a, &b };

	CModuleGraph graph;

	EXPECT_FALSE(graph.Resolve(modules));
}

//////////////////////////////////////////////////////////////////////////
TEST(ModuleGraph, FailsWithAModuleBuiltAgainstADifferentContract)
{
	CFakeModule a{ "A", SVersion{ 1, 0, 0 }, Tge::ModuleContractVersion + 1 };

	Tge::IModule* modules[] = { &a };

	CModuleGraph graph;

	EXPECT_FALSE(graph.Resolve(modules));
}
