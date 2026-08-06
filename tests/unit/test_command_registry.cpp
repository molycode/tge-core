#include <tge/command/registry.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace Tge;

namespace
{
// Every test shares the one process-wide registry, so each cleans up the groups it made.
class CCommandRegistryTest : public ::testing::Test
{
protected:

	void TearDown() override
	{
		for (Command::IGroup* pGroup : m_groups)
		{
			Command::gRegistry->DestroyGroup(pGroup);
		}

		m_groups.clear();
	}

	Command::IGroup* MakeGroup(std::string_view name)
	{
		Command::IGroup* pGroup{ Command::gRegistry->CreateGroup(name, SColor(255, 255, 255)) };
		m_groups.emplace_back(pGroup);

		return pGroup;
	}

	std::vector<Command::IGroup*> m_groups;
};
} // namespace

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, ExecuteReportsFalseForAnUnregisteredVerb)
{
	MakeGroup("units")->RegisterCommand("known", [](std::vector<std::string> const&) {});

	EXPECT_FALSE(Command::gRegistry->Execute("unknown"));
	EXPECT_TRUE(Command::gRegistry->Execute("known"));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, ExecuteReportsFalseForAnEmptyCommandLine)
{
	EXPECT_FALSE(Command::gRegistry->Execute(""));
	EXPECT_FALSE(Command::gRegistry->Execute("   "));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, TheVerbIsPassedAsTheFirstArgument)
{
	std::vector<std::string> received;

	MakeGroup("units")->RegisterCommand("verb", [&received](std::vector<std::string> const& arguments)
	{
		received = arguments;
	});

	EXPECT_TRUE(Command::gRegistry->Execute("verb alpha beta"));

	ASSERT_EQ(received.size(), 3u);
	EXPECT_EQ(received[0], "verb");
	EXPECT_EQ(received[1], "alpha");
	EXPECT_EQ(received[2], "beta");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, RunsOfWhitespaceCollapseRatherThanYieldingEmptyArguments)
{
	std::vector<std::string> received;

	MakeGroup("units")->RegisterCommand("verb", [&received](std::vector<std::string> const& arguments)
	{
		received = arguments;
	});

	EXPECT_TRUE(Command::gRegistry->Execute("  verb \t \t alpha   beta  "));

	ASSERT_EQ(received.size(), 3u);
	EXPECT_EQ(received[0], "verb");
	EXPECT_EQ(received[1], "alpha");
	EXPECT_EQ(received[2], "beta");
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, TheFirstGroupRegisteredClaimsADuplicatedVerb)
{
	bool firstRan{ false };
	bool secondRan{ false };

	MakeGroup("first")->RegisterCommand("shared", [&firstRan](std::vector<std::string> const&)
	{
		firstRan = true;
	});

	MakeGroup("second")->RegisterCommand("shared", [&secondRan](std::vector<std::string> const&)
	{
		secondRan = true;
	});

	EXPECT_TRUE(Command::gRegistry->Execute("shared"));
	EXPECT_TRUE(firstRan);
	EXPECT_FALSE(secondRan);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, ReRegisteringAVerbInOneGroupReplacesTheCallback)
{
	bool originalRan{ false };
	bool replacementRan{ false };

	Command::IGroup* pGroup{ MakeGroup("units") };

	pGroup->RegisterCommand("verb", [&originalRan](std::vector<std::string> const&)
	{
		originalRan = true;
	});

	pGroup->RegisterCommand("verb", [&replacementRan](std::vector<std::string> const&)
	{
		replacementRan = true;
	});

	EXPECT_TRUE(Command::gRegistry->Execute("verb"));
	EXPECT_FALSE(originalRan);
	EXPECT_TRUE(replacementRan);
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, ADestroyedGroupsCommandsStopResolving)
{
	Command::IGroup* pGroup{ Command::gRegistry->CreateGroup("transient", SColor(255, 255, 255)) };
	pGroup->RegisterCommand("transient_verb", [](std::vector<std::string> const&) {});

	EXPECT_TRUE(Command::gRegistry->Execute("transient_verb"));

	Command::gRegistry->DestroyGroup(pGroup);

	EXPECT_FALSE(Command::gRegistry->Execute("transient_verb"));
}

//////////////////////////////////////////////////////////////////////////
TEST_F(CCommandRegistryTest, GetGroupsExposesEachCreatedGroupAndItsCommands)
{
	size_t const before{ Command::gRegistry->GetGroups().size() };

	Command::IGroup* pGroup{ MakeGroup("units") };
	pGroup->RegisterCommand("one", [](std::vector<std::string> const&) {});
	pGroup->RegisterCommand("two", [](std::vector<std::string> const&) {});

	ASSERT_EQ(Command::gRegistry->GetGroups().size(), before + 1u);

	Command::IGroup const* pFound{ nullptr };

	for (Command::IGroup const* pCandidate : Command::gRegistry->GetGroups())
	{
		if (pCandidate->GetName() == "units")
		{
			pFound = pCandidate;
		}
	}

	ASSERT_NE(pFound, nullptr);
	EXPECT_EQ(pFound->GetCommands().size(), 2u);
}
