#pragma once

#include <tge/non_copyable.hpp>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace Tge::Threading
{
class CThreadPool final : private SNoCopyNoMove
{
public:

	explicit CThreadPool(size_t numThreads);
	~CThreadPool();

	void Execute(std::function<void()> work);
	void Terminate();

	size_t GetNumThreads() const { return m_numThreads; }

private:

	void WorkerThread(size_t threadIndex);

	size_t m_numThreads;
	std::vector<std::thread> m_threads;
	std::queue<std::function<void()>> m_workQueue;
	std::mutex m_queueMutex;
	std::condition_variable m_condition;
	std::atomic<bool> m_shouldTerminate{false};
};

extern CThreadPool* g_threadPool;
} // namespace Tge::Threading
