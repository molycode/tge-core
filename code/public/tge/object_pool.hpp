#pragma once

#include "non_copyable.hpp"
#include <tge/assert.hpp>
#include <cstddef>
#include <memory>
#include <vector>
#include <utility>
#include <mutex>

namespace Tge
{

template<typename T, size_t InitialCapacity = 64>
class CObjectPool final : public SNoCopyNoMove
{
public:

	static constexpr size_t MinCapacity = InitialCapacity;
	static constexpr size_t MaxCapacity = 1'048'576;

	CObjectPool() = default;
	~CObjectPool();

	template<typename... Args>
	T* Allocate(Args&&... args);

	void Free(T* object);

	[[nodiscard]] size_t GetCapacity() const;
	[[nodiscard]] size_t GetNumFree() const;
	[[nodiscard]] size_t GetNumUsed() const;

private:

	std::vector<std::unique_ptr<T>> m_objects;
	std::vector<size_t> m_freeIndices;
	mutable std::mutex m_mutex;
};

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t InitialCapacity>
CObjectPool<T, InitialCapacity>::~CObjectPool()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_objects.clear();  // unique_ptr handles deletion
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t InitialCapacity>
template<typename... Args>
T* CObjectPool<T, InitialCapacity>::Allocate(Args&&... args)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// Try to reuse freed slot first
	if (!m_freeIndices.empty())
	{
		size_t idx = m_freeIndices.back();
		m_freeIndices.pop_back();

		m_objects[idx] = std::make_unique<T>(std::forward<Args>(args)...);
		return m_objects[idx].get();
	}

	// Check if we can grow
	TGE_ASSERT(m_objects.size() < MaxCapacity, "ObjectPool exhausted");

	// Grow: add new allocation
	m_objects.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	return m_objects.back().get();
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t InitialCapacity>
void CObjectPool<T, InitialCapacity>::Free(T* object)
{
	if (object != nullptr)
	{
	 std::lock_guard<std::mutex> lock(m_mutex);

		// Find the object
		for (size_t i = 0; i < m_objects.size(); ++i)
		{
			if (m_objects[i] && m_objects[i].get() == object)
			{
				m_objects[i].reset();  // Delete the object
				m_freeIndices.push_back(i);  // Mark slot as free
				return;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t InitialCapacity>
size_t CObjectPool<T, InitialCapacity>::GetCapacity() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_objects.size();
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t InitialCapacity>
size_t CObjectPool<T, InitialCapacity>::GetNumFree() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_freeIndices.size();
}

//////////////////////////////////////////////////////////////////////////
template<typename T, size_t InitialCapacity>
size_t CObjectPool<T, InitialCapacity>::GetNumUsed() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_objects.size() - m_freeIndices.size();
}
} // namespace Tge
