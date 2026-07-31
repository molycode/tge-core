#include "group.hpp"

#include <tge/logging/loggers.hpp>

namespace Tge::Command
{
//////////////////////////////////////////////////////////////////////////
CGroup::CGroup(std::string_view name, SColor const& color)
	: m_name{ name }
	, m_color{ color }
{
}

//////////////////////////////////////////////////////////////////////////
void CGroup::RegisterCommand(std::string_view name, Callback callback)
{
	std::string verb{ name };

	if (m_commands.contains(verb))
	{
		gLog.Warning("'{}' is already registered in group '{}' and will be replaced", verb, m_name);
	}

	m_commands[std::move(verb)] = std::move(callback);
}
} // namespace Tge::Command
