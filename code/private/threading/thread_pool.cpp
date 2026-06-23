#include "thread_pool.hpp"
#include <tge/threading/job_system.hpp>
#include <tge/threading/thread_name.hpp>
#include <format>

namespace Tge::Threading
{
CThreadPool* gThreadPool = nullptr;

//////////////////////////////////////////////////////////////////////////
CThreadPool::CThreadPool(size_t numThreads)
	: m_numThreads(numThreads)
{
	m_threads.reserve(numThreads);

	for (size_t i = 0; i < numThreads; ++i)
	{
		m_threads.emplace_back(&CThreadPool::WorkerThread, this, i);
	}
}

//////////////////////////////////////////////////////////////////////////
CThreadPool::~CThreadPool()
{
	Terminate();
}

//////////////////////////////////////////////////////////////////////////
void CThreadPool::Execute(std::function<void()> work)
{
	if (work)
	{
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_workQueue.push(std::move(work));
		}

		m_condition.notify_one();
	}
}

//////////////////////////////////////////////////////////////////////////
void CThreadPool::Terminate()
{
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_shouldTerminate.store(true, std::memory_order_relaxed);
	}

	m_condition.notify_all();

	for (std::thread& thread : m_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	m_threads.clear();
}

//////////////////////////////////////////////////////////////////////////
void CThreadPool::WorkerThread(size_t threadIndex)
{
	InitializeThread();
	SetCurrentThreadName(std::format("TgeWorker {}", threadIndex).c_str());

	while (true)
	{
		std::function<void()> work;

		{
			std::unique_lock<std::mutex> lock(m_queueMutex);

			m_condition.wait(lock, [this]()
			{
				return m_shouldTerminate.load() || !m_workQueue.empty();
			});

			if (m_shouldTerminate.load() && m_workQueue.empty())
			{
				break;
			}

			if (!m_workQueue.empty())
			{
				work = std::move(m_workQueue.front());
				m_workQueue.pop();
			}
		}

		if (work)
		{
			work();
		}
	}

	FinalizeThread();
}
} // namespace Tge::Threading
