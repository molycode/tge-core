#pragma once

#include <tge/non_copyable.hpp>
#include <tge/threading/job.hpp>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <functional>

namespace Tge::Threading
{
class CThreadPool final : private SNoCopyNoMove
{
public:

	explicit CThreadPool(size_t numThreads);
	~CThreadPool();

	void Execute(std::function<void()> work, EJobPriority priority = EJobPriority::Normal);
	void Terminate();

	size_t GetNumThreads() const { return m_numThreads; }

private:

	struct SWorkItem final
	{
		std::function<void()> work;
		EJobPriority priority = EJobPriority::Normal;
		uint64_t sequence = 0;

		// priority_queue pops the greatest element, so invert: High (numerically lowest) must win. Equal
		// priorities fall back to insertion order, keeping submission-order FIFO within a priority band.
		bool operator<(SWorkItem const& other) const
		{
			return (priority != other.priority) ? (priority > other.priority) : (sequence > other.sequence);
		}
	};

	void WorkerThread(size_t threadIndex);

	size_t m_numThreads;
	std::vector<std::thread> m_threads;
	std::priority_queue<SWorkItem> m_workQueue;
	uint64_t m_nextSequence = 0;
	std::mutex m_queueMutex;
	std::condition_variable m_condition;
	std::atomic<bool> m_shouldTerminate{false};
};

extern CThreadPool* gThreadPool;
} // namespace Tge::Threading
