#pragma once

#include <tge/command/registry.hpp>
#include <vector>

namespace Tge::Command
{
class CRegistry final : public IRegistry
{
public:

	CRegistry() = default;
	~CRegistry() = default;

	// Tge::Command::IRegistry
	IGroup* CreateGroup(std::string_view name, SColor const& color) override;
	void DestroyGroup(IGroup* pGroup) override;
	bool Execute(std::string_view commandLine) override;
	std::vector<IGroup*> const& GetGroups() const override { return m_groups; }
	// ~Tge::Command::IRegistry

	void Terminate();

private:

	std::vector<IGroup*> m_groups;
};
} // namespace Tge::Command
