#pragma once

#include "non_copyable.hpp"
#include <tge/init/assert.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace Tge
{

template<typename T, size_t Capacity = 64>
class CObjectPool final : private SNoCopyNoMove
{
public:

	static constexpr size_t PoolCapacity = Capacity;

	CObjectPool();
	~CObjectPool();

	template<typename... Args>
	T* Allocate(Args&&... args);

	void Free(T* object);

	size_t GetCapacity() const { return Capacity; }
	size_t GetNumFree() const;
	size_t GetNumUsed() const;

private:

	struct alignas(alignof(T)) SSlot final
	{
		std::byte storage[sizeof(T)];
	};

	std::unique_ptr<SSlot[]> m_slots;
	std::unique_ptr<bool[]>  m_alive;
	std::vector<size_t>      m_freeIndices;
	mutable std::mutex       m_mutex;
};

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t Capacity>
CObjectPool<T, Capacity>::CObjectPool()
	: m_slots(std::make_unique<SSlot[]>(Capacity))
	, m_alive(std::make_unique<bool[]>(Capacity))
{
	m_freeIndices.reserve(Capacity);

	for (size_t i = Capacity; i > 0; --i)
	{
		m_freeIndices.push_back(i - 1);
	}
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t Capacity>
CObjectPool<T, Capacity>::~CObjectPool()
{
	for (size_t i = 0; i < Capacity; ++i)
	{
		if (m_alive[i])
		{
			reinterpret_cast<T*>(m_slots[i].storage)->~T();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t Capacity>
template<typename... Args>
T* CObjectPool<T, Capacity>::Allocate(Args&&... args)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	TGE_ASSERT(!m_freeIndices.empty(), "CObjectPool exhausted: no free slots remaining");

	size_t const idx = m_freeIndices.back();
	m_freeIndices.pop_back();

	T* obj = new(m_slots[idx].storage) T(std::forward<Args>(args)...);
	m_alive[idx] = true;
	return obj;
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t Capacity>
void CObjectPool<T, Capacity>::Free(T* object)
{
	if (object != nullptr)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		size_t const idx = reinterpret_cast<SSlot*>(object) - m_slots.get();
		TGE_ASSERT(idx < Capacity && m_alive[idx], "CObjectPool::Free: pointer does not belong to this pool or slot is not alive");

		object->~T();
		m_alive[idx] = false;
		m_freeIndices.push_back(idx);
	}
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t Capacity>
size_t CObjectPool<T, Capacity>::GetNumFree() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_freeIndices.size();
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t Capacity>
size_t CObjectPool<T, Capacity>::GetNumUsed() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return Capacity - m_freeIndices.size();
}
} // namespace Tge
