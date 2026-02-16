#include <tge/memory/category_context.hpp>

#ifdef TGE_MEMORY_TRACKING_ENABLED

namespace Tge::Memory
{
namespace
{
	thread_local ECategory g_currentCategory = ECategory::Global;
}

//////////////////////////////////////////////////////////////////////////
ECategory GetCurrentCategory()
{
	return g_currentCategory;
}

//////////////////////////////////////////////////////////////////////////
void SetCurrentCategory(ECategory category)
{
	g_currentCategory = category;
}

//////////////////////////////////////////////////////////////////////////
CScopedCategory::CScopedCategory(ECategory category)
	: m_previousCategory(g_currentCategory)
{
	g_currentCategory = category;
}

//////////////////////////////////////////////////////////////////////////
CScopedCategory::~CScopedCategory()
{
	g_currentCategory = m_previousCategory;
}

} // namespace Tge::Memory

#endif // TGE_MEMORY_TRACKING_ENABLED
