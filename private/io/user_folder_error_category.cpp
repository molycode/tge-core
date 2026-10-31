#include "user_folder_error_category.hpp"

#if defined(TGE_PLATFORM_LINUX)
namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
char const* CUserFolderErrorCategory::name() const noexcept
{
	return "user folder";
}

//////////////////////////////////////////////////////////////////////////
std::string CUserFolderErrorCategory::message(int value) const
{
	return (value == NoHomeError) ? "HOME is unset or not an absolute path" : "unknown user folder error";
}

//////////////////////////////////////////////////////////////////////////
std::error_category const& GetUserFolderErrorCategory()
{
	static CUserFolderErrorCategory const category{};
	return category;
}
} // namespace Tge::IO
#endif // defined(TGE_PLATFORM_LINUX)
