// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

// C++ implementation of Dmitry Vyukov's non-intrusive lock free unbound MPSC queue
// http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
//
// Modernized for C++23:
// - std::aligned_storage (deprecated) -> alignas
// - NULL -> nullptr
// - Added cache-line alignment to prevent false sharing

#pragma once

#include <atomic>

template<typename T>
class CMpscQueue final
{
public:
	CMpscQueue() :
		m_head(reinterpret_cast<SBufferNode*>(new SBufferNodeAligned)),
		m_tail(m_head.load(std::memory_order_relaxed))
	{
		SBufferNode* front = m_head.load(std::memory_order_relaxed);
		front->next.store(nullptr, std::memory_order_relaxed);
	}

	~CMpscQueue()
	{
		T output;
		while (Dequeue(output)) {}
		SBufferNode* front = m_head.load(std::memory_order_relaxed);
		delete reinterpret_cast<SBufferNodeAligned*>(front);
	}

	void Enqueue(T const& input)
	{
		SBufferNode* node = reinterpret_cast<SBufferNode*>(new SBufferNodeAligned);
		node->data = input;
		node->next.store(nullptr, std::memory_order_relaxed);

		SBufferNode* prevHead = m_head.exchange(node, std::memory_order_acq_rel);
		prevHead->next.store(node, std::memory_order_release);
	}

	bool Dequeue(T& output)
	{
		SBufferNode* tail = m_tail.load(std::memory_order_relaxed);
		SBufferNode* next = tail->next.load(std::memory_order_acquire);

		if (next == nullptr)
		{
			return false;
		}

		output = next->data;
		m_tail.store(next, std::memory_order_release);
		delete reinterpret_cast<SBufferNodeAligned*>(tail);
		return true;
	}

	CMpscQueue(CMpscQueue const&) = delete;
	CMpscQueue& operator=(CMpscQueue const&) = delete;

private:
	// Standard cache line size for x86_64 (stable value for ABI compatibility)
	static constexpr size_t CacheLineSize = 64;

	struct SBufferNode
	{
		T data;
		std::atomic<SBufferNode*> next;
	};

	// Align to cache line boundary to prevent false sharing
	struct alignas(CacheLineSize) SBufferNodeAligned
	{
		SBufferNode node;
	};

	// Cache-line aligned atomics to prevent false sharing between head and tail
	alignas(CacheLineSize) std::atomic<SBufferNode*> m_head;
	alignas(CacheLineSize) std::atomic<SBufferNode*> m_tail;
};
