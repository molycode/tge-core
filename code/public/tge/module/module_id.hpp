#pragma once

#include <tge/module/module.hpp>
#include <string_view>

namespace Tge
{
class CModuleId final : public IModuleId
{
public:

	constexpr explicit CModuleId(std::string_view name) : m_name{ name } {}
	~CModuleId() = default;

	// Tge::IModuleId
	std::string_view GetName() const override { return m_name; }
	// ~Tge::IModuleId

private:

	std::string_view m_name;
};
} // namespace Tge
