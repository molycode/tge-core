#include <tge/memory/category_context.hpp>

#ifdef TGE_MEMORY_TRACKING_ENABLED

namespace Tge::Memory
{
namespace
{
	thread_local CategoryId gCurrentCategory = Category::Global;
} // namespace

//////////////////////////////////////////////////////////////////////////
CategoryId GetCurrentCategory()
{
	return gCurrentCategory;
}

//////////////////////////////////////////////////////////////////////////
void SetCurrentCategory(CategoryId category)
{
	gCurrentCategory = category;
}

//////////////////////////////////////////////////////////////////////////
CScopedCategory::CScopedCategory(CategoryId category)
	: m_previousCategory(gCurrentCategory)
{
	gCurrentCategory = category;
}

//////////////////////////////////////////////////////////////////////////
CScopedCategory::~CScopedCategory()
{
	gCurrentCategory = m_previousCategory;
}

} // namespace Tge::Memory

#endif // TGE_MEMORY_TRACKING_ENABLED
