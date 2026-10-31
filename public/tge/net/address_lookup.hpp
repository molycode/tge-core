#pragma once

#include <tge/non_copyable.hpp>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>

namespace Tge::Net
{
struct SNativeLookup;

class CAddressLookup final : private SNoCopyNoMove
{
public:

	CAddressLookup();
	~CAddressLookup();

	std::expected<void, std::error_code> Start(std::string_view host);
	std::optional<std::expected<uint32_t, std::error_code>> Poll();
	void Terminate();

private:

	std::unique_ptr<SNativeLookup> m_pLookup{};
};
} // namespace Tge::Net
