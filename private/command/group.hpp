#pragma once

#include <tge/command/group.hpp>

namespace Tge::Command
{
class CGroup final : public IGroup
{
public:

	explicit CGroup(std::string_view name, SColor const& color);
	~CGroup() = default;

	// Tge::Command::IGroup
	void RegisterCommand(std::string_view name, Callback callback) override;
	std::string const& GetName() const override { return m_name; }
	SColor const& GetColor() const override { return m_color; }
	std::map<std::string, Callback> const& GetCommands() const override { return m_commands; }
	// ~Tge::Command::IGroup

private:

	std::string m_name;
	std::map<std::string, Callback> m_commands;
	SColor m_color;
};
} // namespace Tge::Command
