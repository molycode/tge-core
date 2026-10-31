#include "lookup_error_category.hpp"

#if defined(TGE_PLATFORM_LINUX)
#include <netdb.h>

namespace Tge::Net
{
//////////////////////////////////////////////////////////////////////////
char const* CLookupErrorCategory::name() const noexcept
{
	return "address lookup";
}

//////////////////////////////////////////////////////////////////////////
std::string CLookupErrorCategory::message(int value) const
{
	return gai_strerror(value);
}

//////////////////////////////////////////////////////////////////////////
std::error_category const& GetLookupErrorCategory()
{
	static CLookupErrorCategory const category{};
	return category;
}
} // namespace Tge::Net
#endif // defined(TGE_PLATFORM_LINUX)
