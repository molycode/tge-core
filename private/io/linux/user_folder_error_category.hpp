#pragma once

#include <string>
#include <system_error>

namespace Tge::IO
{
inline constexpr int NoHomeError{ 1 };

class CUserFolderErrorCategory final : public std::error_category
{
public:

	CUserFolderErrorCategory() = default;
	~CUserFolderErrorCategory() override = default;

	char const* name() const noexcept override;
	std::string message(int value) const override;
};

std::error_category const& GetUserFolderErrorCategory();
} // namespace Tge::IO
