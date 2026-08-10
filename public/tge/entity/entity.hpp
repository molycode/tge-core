#pragma once

#include <tge/index.hpp>

#include <functional>
#include <cstddef>
#include <cstdint>

namespace Tge::Entity
{
// Stable, generation-checked handle to an entity. Parent/child links and every
// external reference store an SEntity rather than a pointer, so they survive
// slot reuse and dense-array compaction; a stale handle is detected via the
// generation counter (a reused slot bumps its generation).
struct SEntity final
{
	uint32_t index{ InvalidIndex };
	uint32_t generation{ 0 };

	bool IsValid() const { return index != InvalidIndex; }

	bool operator==(SEntity const&) const = default;
};

static_assert(sizeof(SEntity) == 8 && alignof(SEntity) == 4);
} // namespace Tge::Entity

template<>
struct std::hash<Tge::Entity::SEntity>
{
	size_t operator()(Tge::Entity::SEntity const& entity) const noexcept
	{
		uint64_t const packed = (static_cast<uint64_t>(entity.generation) << 32)
			| static_cast<uint64_t>(entity.index);

		return std::hash<uint64_t>{}(packed);
	}
};
