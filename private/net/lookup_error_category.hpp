#pragma once

#include <tge/platform.hpp>
#include <string>
#include <system_error>

#if defined(TGE_PLATFORM_LINUX)
namespace Tge::Net
{
class CLookupErrorCategory final : public std::error_category
{
public:

	CLookupErrorCategory() = default;
	~CLookupErrorCategory() override = default;

	char const* name() const noexcept override;
	std::string message(int value) const override;
};

std::error_category const& GetLookupErrorCategory();
} // namespace Tge::Net
#endif // defined(TGE_PLATFORM_LINUX)
