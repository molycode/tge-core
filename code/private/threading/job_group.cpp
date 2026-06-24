#include <tge/threading/job_group.hpp>
#include "job_scheduler.hpp"
#include <chrono>

namespace Tge::Threading
{
CJobGroup gDefaultJobGroup;

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
		job->Execute();
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
} // namespace Tge::Threading
