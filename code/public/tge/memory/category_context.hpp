#pragma once

#include <tge/memory/tracking.hpp>

namespace Tge::Memory
{
#ifdef TGE_MEMORY_TRACKING_ENABLED

// Get/Set thread-local current category (tracking enabled)
CategoryId GetCurrentCategory();
void SetCurrentCategory(CategoryId category);

// RAII helper for scoped category tracking (tracking enabled)
class CScopedCategory final
{
public:

	explicit CScopedCategory(CategoryId category);
	~CScopedCategory();

private:

	CategoryId m_previousCategory;
};

#else

// No-op implementations when tracking disabled (zero overhead)
inline CategoryId GetCurrentCategory() { return Category::Global; }
inline void SetCurrentCategory(CategoryId) {}

class CScopedCategory final
{
public:

	explicit CScopedCategory(CategoryId) {}
	~CScopedCategory() {}
};

#endif // TGE_MEMORY_TRACKING_ENABLED
} // namespace Tge::Memory
