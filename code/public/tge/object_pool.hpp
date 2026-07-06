#pragma once

#include "non_copyable.hpp"
#include <tge/assert.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace Tge
{

template<typename T, size_t BlockSize = 64>
class CObjectPool final : private SNoCopyNoMove
{
public:

	static constexpr size_t SlotsPerBlock = BlockSize;

	CObjectPool() = default;
	~CObjectPool();

	template<typename... Args>
	T* Allocate(Args&&... args);

	void Free(T* object);

	size_t GetCapacity() const;
	size_t GetNumFree() const;
	size_t GetNumUsed() const;

private:

	struct alignas(alignof(T)) SSlot final
	{
		std::byte storage[sizeof(T)];
		bool      used{ false };
	};

	SSlot* SlotAt(size_t index) const;

	// Slots live in fixed-size blocks that never move, so pointers handed out by
	// Allocate stay valid across growth.
	std::vector<std::unique_ptr<SSlot[]>> m_blocks;
	std::vector<size_t>                   m_freeIndices;
	size_t                                m_size{ 0 };
	mutable std::mutex                    m_mutex;
};

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t BlockSize>
CObjectPool<T, BlockSize>::~CObjectPool()
{
	for (size_t i = 0; i < m_size; ++i)
	{
		SSlot* slot = SlotAt(i);

		if (slot->used)
		{
			reinterpret_cast<T*>(slot->storage)->~T();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t BlockSize>
typename CObjectPool<T, BlockSize>::SSlot* CObjectPool<T, BlockSize>::SlotAt(size_t index) const
{
	return &m_blocks[index / SlotsPerBlock][index % SlotsPerBlock];
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t BlockSize>
template<typename... Args>
T* CObjectPool<T, BlockSize>::Allocate(Args&&... args)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	size_t idx{ 0 };

	if (!m_freeIndices.empty())
	{
		idx = m_freeIndices.back();
		m_freeIndices.pop_back();
	}
	else
	{
		if (m_size == m_blocks.size() * SlotsPerBlock)
		{
			m_blocks.push_back(std::make_unique<SSlot[]>(SlotsPerBlock));
		}

		idx = m_size;
		++m_size;
	}

	SSlot* slot = SlotAt(idx);
	T* obj = new(slot->storage) T(std::forward<Args>(args)...);
	slot->used = true;
	return obj;
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t BlockSize>
void CObjectPool<T, BlockSize>::Free(T* object)
{
	if (object != nullptr)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		SSlot* target = reinterpret_cast<SSlot*>(object);
		size_t blockIndex{ 0 };
		bool   found{ false };

		for (size_t b = 0; b < m_blocks.size() && !found; ++b)
		{
			SSlot* base = m_blocks[b].get();

			if (target >= base && target < base + SlotsPerBlock)
			{
				blockIndex = b;
				found = true;
			}
		}

		TGE_ASSERT(found, "CObjectPool::Free: pointer does not belong to this pool");

		if (found)
		{
			size_t const local = static_cast<size_t>(target - m_blocks[blockIndex].get());

			TGE_ASSERT(target->used, "CObjectPool::Free: slot is not alive (double free?)");

			object->~T();
			target->used = false;
			m_freeIndices.push_back(blockIndex * SlotsPerBlock + local);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t BlockSize>
size_t CObjectPool<T, BlockSize>::GetCapacity() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_size;
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t BlockSize>
size_t CObjectPool<T, BlockSize>::GetNumFree() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_freeIndices.size();
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t BlockSize>
size_t CObjectPool<T, BlockSize>::GetNumUsed() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_size - m_freeIndices.size();
}
} // namespace Tge
