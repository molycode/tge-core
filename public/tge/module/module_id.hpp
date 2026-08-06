#pragma once

#include <tge/module/module.hpp>
#include <string_view>

namespace Tge
{
class CModuleId final : public IModuleId
{
public:

	// contractVersion is a default argument so it is evaluated in the MODULE's translation unit; an inline
	// virtual returning the constant would be deduped by the linker and every module would report one value.
	constexpr explicit CModuleId(std::string_view name, SVersion version,
	                             uint32_t contractVersion = ModuleContractVersion)
		: m_name{ name }
		, m_version{ version }
		, m_contractVersion{ contractVersion }
	{}
	~CModuleId() = default;

	// Tge::IModuleId
	std::string_view GetName() const override { return m_name; }
	SVersion GetVersion() const override { return m_version; }
	uint32_t GetContractVersion() const override { return m_contractVersion; }
	// ~Tge::IModuleId

private:

	std::string_view m_name;
	SVersion         m_version;
	uint32_t         m_contractVersion;
};
} // namespace Tge
