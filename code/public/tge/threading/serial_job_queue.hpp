#pragma once

#include <tge/non_copyable.hpp>
#include <tge/threading/thread_name.hpp>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>

namespace Tge::Threading
{
template<typename TResult>
class CSerialJobQueue final : private Tge::SNoCopyNoMove
{
public:

	CSerialJobQueue() = default;

	~CSerialJobQueue()
	{
		Stop();
	}

	void Start()
	{
		if (!m_isRunning.load())
		{
			m_shouldStop.store(false);
			m_isRunning.store(true);
			m_thread = std::thread(&CSerialJobQueue::WorkerThreadMain, this);
		}
	}

	void Stop()
	{
		if (m_isRunning.load())
		{
			m_shouldStop.store(true);
			m_workAvailable.notify_one();

			if (m_thread.joinable())
			{
				m_thread.join();
			}

			m_isRunning.store(false);
		}
	}

	// Stop the worker (blocks until any in-progress job finishes), clear all internal
	// state (pending/cancelled/completed), then restart. Releases all captured references.
	void Flush()
	{
		Stop();
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_pending.clear();
			m_cancelled.clear();
			m_completed.clear();
		}
		Start();
	}

	bool IsRunning() const
	{
		return m_isRunning.load();
	}

	// Non-blocking submission. jobId used for cancellation.
	void Submit(std::string_view jobId, std::function<TResult()> work)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_pending.push_back(SEntry{std::string{jobId}, std::move(work)});
		}

		m_workAvailable.notify_one();
	}

	// Non-blocking result poll. Returns nullopt when queue is empty.
	std::optional<TResult> Poll()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::optional<TResult> result;

		if (!m_completed.empty())
		{
			result = std::move(m_completed.front());
			m_completed.pop_front();
		}

		return result;
	}

	// Removes pending job with matching jobId; marks in-flight job cancelled
	// (its result is discarded when it completes).
	void Cancel(std::string_view jobId)
	{
		std::string const jobIdStr{jobId};
		std::lock_guard<std::mutex> lock(m_mutex);

		auto removeIt = std::remove_if(m_pending.begin(), m_pending.end(),
			[&jobIdStr](SEntry const& e) { return e.jobId == jobIdStr; });

		bool const wasPending = (removeIt != m_pending.end());
		m_pending.erase(removeIt, m_pending.end());

		if (!wasPending)
		{
			m_cancelled.insert(jobIdStr);
		}
	}

	bool HasPendingWork() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return !m_pending.empty();
	}

	size_t GetPendingCount() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_pending.size();
	}

private:

	struct SEntry final
	{
		std::string jobId;
		std::function<TResult()> work;
	};

	void WorkerThreadMain()
	{
		SetCurrentThreadName("TgeSerialJob");

		bool shouldExit = false;

		while (!shouldExit)
		{
			SEntry entry;
			bool shouldProcess = false;

			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_workAvailable.wait(lock, [this]()
				{
					return m_shouldStop.load() || !m_pending.empty();
				});

				shouldExit = m_shouldStop.load();

				if (!shouldExit)
				{
					entry = std::move(m_pending.front());
					m_pending.pop_front();

					bool const cancelled = (m_cancelled.count(entry.jobId) > 0);

					if (cancelled)
					{
						m_cancelled.erase(entry.jobId);
					}

					shouldProcess = !cancelled;
				}
			}

			if (shouldProcess)
			{
				TResult result = entry.work();
				std::lock_guard<std::mutex> lock(m_mutex);
				m_completed.push_back(std::move(result));
			}
		}
	}

	std::thread             m_thread;
	std::atomic<bool>       m_isRunning{false};
	std::atomic<bool>       m_shouldStop{false};
	std::deque<SEntry>      m_pending;
	std::deque<TResult>     m_completed;
	std::set<std::string>   m_cancelled;
	mutable std::mutex      m_mutex;
	std::condition_variable m_workAvailable;
};
} // namespace Tge::Threading
