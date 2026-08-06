#pragma once

#include <tge/color.hpp>
#include <tge/non_copyable.hpp>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace Tge::Command
{
using Callback = std::function<void(std::vector<std::string> const& arguments)>;

// A named, coloured set of commands. A subsystem owns one and registers its verbs into it; the frontend that
// renders the registry reads the name and colour to group them.
class IGroup : public SNoCopyNoMove
{
public:

	virtual void RegisterCommand(std::string_view name, Callback callback) = 0;

	virtual std::string const& GetName() const = 0;
	virtual SColor const& GetColor() const = 0;
	virtual std::map<std::string, Callback> const& GetCommands() const = 0;

protected:

	~IGroup() = default;
};
} // namespace Tge::Command
