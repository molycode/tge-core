#pragma once

#include <tge/memory/tracking.hpp>

namespace Tge::Memory
{
#ifdef TGE_MEMORY_TRACKING_ENABLED

// Get/Set thread-local current category (tracking enabled)
ECategory GetCurrentCategory();
void SetCurrentCategory(ECategory category);

// RAII helper for scoped category tracking (tracking enabled)
class CScopedCategory final
{
public:

	explicit CScopedCategory(ECategory category);
	~CScopedCategory();

private:

	ECategory m_previousCategory;
};

#else

// No-op implementations when tracking disabled (zero overhead)
inline ECategory GetCurrentCategory() { return ECategory::Global; }
inline void SetCurrentCategory(ECategory) {}

class CScopedCategory final
{
public:

	explicit CScopedCategory(ECategory) {}
	~CScopedCategory() {}
};

#endif // TGE_MEMORY_TRACKING_ENABLED
} // namespace Tge::Memory
