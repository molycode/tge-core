#include "registry.hpp"

#include "group.hpp"

#include <tge/logging/loggers.hpp>
#include <tge/memory/tracking.hpp>
#include <algorithm>
#include <string>

namespace Tge::Command
{
namespace
{
Memory::CategoryId const GroupCategory{ Memory::RegisterCategory("Command") };

std::vector<std::string> Tokenize(std::string_view commandLine)
{
	std::vector<std::string> tokens;
	std::string token;

	for (char const character : commandLine)
	{
		if (character == ' ' || character == '\t')
		{
			if (!token.empty())
			{
				tokens.emplace_back(std::move(token));
				token.clear();
			}
		}
		else
		{
			token += character;
		}
	}

	if (!token.empty())
	{
		tokens.emplace_back(std::move(token));
	}

	return tokens;
}

CRegistry gRegistryImpl;
} // namespace

extern IRegistry* const gRegistry = static_cast<IRegistry*>(&gRegistryImpl);

//////////////////////////////////////////////////////////////////////////
IGroup* CRegistry::CreateGroup(std::string_view name, SColor const& color)
{
	Memory::TrackAllocation(GroupCategory, sizeof(CGroup));

	CGroup* pGroup{ new CGroup(name, color) };
	m_groups.emplace_back(pGroup);

	return pGroup;
}

//////////////////////////////////////////////////////////////////////////
void CRegistry::DestroyGroup(IGroup* pGroup)
{
	auto const it{ std::ranges::find(m_groups, pGroup) };

	if (it != m_groups.end())
	{
		Memory::TrackDeallocation(GroupCategory, sizeof(CGroup));

		// Every entry was created by CreateGroup, so the concrete type is known.
		delete static_cast<CGroup*>(*it);
		m_groups.erase(it);
	}
	else
	{
		gLog.Warning("Cannot destroy command group: it was not created by this registry");
	}
}

//////////////////////////////////////////////////////////////////////////
bool CRegistry::Execute(std::string_view commandLine)
{
	std::vector<std::string> const tokens{ Tokenize(commandLine) };
	bool executed{ false };

	if (!tokens.empty())
	{
		std::string const& verb{ tokens[0] };

		for (IGroup const* pGroup : m_groups)
		{
			if (!executed)
			{
				auto const& commands{ pGroup->GetCommands() };
				auto const it{ commands.find(verb) };

				if (it != commands.end())
				{
					it->second(tokens);
					executed = true;
				}
			}
		}
	}

	return executed;
}

//////////////////////////////////////////////////////////////////////////
void CRegistry::Terminate()
{
	if (!m_groups.empty())
	{
		gLog.Warning("{} command group(s) outlived the registry:", m_groups.size());

		for (IGroup const* pGroup : m_groups)
		{
			gLog.Warning("  - '{}'", pGroup->GetName());
		}
	}

	for (IGroup* pGroup : m_groups)
	{
		Memory::TrackDeallocation(GroupCategory, sizeof(CGroup));
		delete static_cast<CGroup*>(pGroup);
	}

	m_groups.clear();
}

namespace Internal
{
//////////////////////////////////////////////////////////////////////////
void Terminate()
{
	gRegistryImpl.Terminate();
}
} // namespace Internal
} // namespace Tge::Command
