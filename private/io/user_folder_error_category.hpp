#pragma once

#include <tge/platform.hpp>
#include <string>
#include <system_error>

#if defined(TGE_PLATFORM_LINUX)
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
#endif // defined(TGE_PLATFORM_LINUX)
