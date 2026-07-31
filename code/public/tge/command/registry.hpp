#pragma once

#include <tge/command/group.hpp>
#include <tge/color.hpp>
#include <tge/non_copyable.hpp>
#include <string_view>
#include <vector>

namespace Tge::Command
{
// Registration and dispatch, with no frontend of its own: an ImGui window, a terminal and a command-line flag
// all drive the same table.
class IRegistry : public SNoCopyNoMove
{
public:

	virtual IGroup* CreateGroup(std::string_view name, SColor const& color) = 0;
	virtual void DestroyGroup(IGroup* pGroup) = 0;

	// False when no group claimed the verb, so a caller that cannot see the frontend's output can fail
	// instead of assuming its command landed.
	virtual bool Execute(std::string_view commandLine) = 0;

	virtual std::vector<IGroup*> const& GetGroups() const = 0;

protected:

	~IRegistry() = default;
};

extern IRegistry* const gRegistry;
} // namespace Tge::Command
