#include <tge/threading/job_group.hpp>
#include "job_scheduler.hpp"
#include <chrono>

namespace Tge::Threading
{
thread_local CJobGroup::SState* CJobGroup::SState::tCurrent = nullptr;

//////////////////////////////////////////////////////////////////////////
CJobGroup::CJobGroup()
{
	// Inherit the currently-running group as cancellation parent, so a group created inside a job
	// (a nested bake stage) is cancelled together with its parent — no cancellation handle to thread through.
	if (SState::tCurrent != nullptr)
	{
		m_state->parent = SState::tCurrent->shared_from_this();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CJobGroup::SState::IsCancelled() const
{
	return cancelled.load(std::memory_order_relaxed) || (parent && parent->IsCancelled());
}

//////////////////////////////////////////////////////////////////////////
bool CJobGroup::SState::RunOnePending()
{
	std::unique_ptr<IJob> job;

	{
		std::lock_guard<std::mutex> lock(mutex);

		if (!pending.empty())
		{
			job = std::move(pending.front());
			pending.pop_front();
		}
	}

	bool const ran = (job != nullptr);

	if (ran)
	{
		// A cancelled group drains its queue without running the bodies, so Wait() returns promptly.
		if (!IsCancelled())
		{
			SState* const prev = tCurrent;
			tCurrent = this;
			job->Execute();
			tCurrent = prev;
		}

		activeJobs.fetch_sub(1, std::memory_order_relaxed);
		completion.notify_all();
	}

	return ran;
}

//////////////////////////////////////////////////////////////////////////
void CJobGroup::SubmitJob(std::unique_ptr<IJob> job, EJobPriority priority)
{
	if (job && gJobScheduler)
	{
		m_state->activeJobs.fetch_add(1, std::memory_order_relaxed);

		{
			std::lock_guard<std::mutex> lock(m_state->mutex);
			m_state->pending.push_back(std::move(job));
		}

		// Runner drains one of this group's jobs; the captured state ref keeps it alive past the handle.
		std::shared_ptr<SState> state = m_state;
		auto runner = [state]() { state->RunOnePending(); };
		gJobScheduler->SubmitJob(std::unique_ptr<IJob>(new CLambdaJob<decltype(runner)>(std::move(runner))), priority);
	}
}

//////////////////////////////////////////////////////////////////////////
void CJobGroup::Wait()
{
	// Run only this group's own jobs while waiting — stealing unrelated pool jobs here can deadlock.
	while (m_state->activeJobs.load(std::memory_order_acquire) != 0)
	{
		if (!m_state->RunOnePending())
		{
			// Nothing left to run inline; block briefly (timeout bounds a missed wakeup).
			std::unique_lock<std::mutex> lock(m_state->mutex);

			m_state->completion.wait_for(lock, std::chrono::milliseconds(1), [this]()
			{
				return m_state->activeJobs.load(std::memory_order_acquire) == 0;
			});
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CJobGroup::IsComplete() const
{
	return m_state->activeJobs.load(std::memory_order_relaxed) == 0;
}

//////////////////////////////////////////////////////////////////////////
size_t CJobGroup::GetActiveCount() const
{
	return m_state->activeJobs.load(std::memory_order_relaxed);
}

//////////////////////////////////////////////////////////////////////////
void CJobGroup::Cancel()
{
	m_state->cancelled.store(true, std::memory_order_relaxed);

	// Drop still-queued jobs so Wait() drains to zero without running them; the mutex serialises
	// against RunOnePending so each job's activeJobs count is settled exactly once (popped or dropped).
	std::deque<std::unique_ptr<IJob>> dropped;

	{
		std::lock_guard<std::mutex> lock(m_state->mutex);
		dropped.swap(m_state->pending);
	}

	if (!dropped.empty())
	{
		m_state->activeJobs.fetch_sub(dropped.size(), std::memory_order_relaxed);
		m_state->completion.notify_all();
	}
}

//////////////////////////////////////////////////////////////////////////
bool CJobGroup::IsCancellationRequested() const
{
	return m_state->IsCancelled();
}

//////////////////////////////////////////////////////////////////////////
bool IsCurrentJobCancelled()
{
	return CJobGroup::SState::tCurrent != nullptr && CJobGroup::SState::tCurrent->IsCancelled();
}
} // namespace Tge::Threading
