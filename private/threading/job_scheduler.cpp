#include "job_scheduler.hpp"
#include "thread_pool.hpp"
#include <tge/assert.hpp>

namespace Tge::Threading
{
CJobScheduler* gJobScheduler = nullptr;

//////////////////////////////////////////////////////////////////////////
void CJobScheduler::SubmitJob(std::unique_ptr<IJob> job, EJobPriority priority)
{
	// Dropping the job here would leave CJobGroup's activeJobs counter raised, hanging its Wait() forever.
	TGE_ASSERT(gThreadPool != nullptr, "Job submitted before the thread pool was initialized");

	if (job && gThreadPool)
	{
		m_pendingJobs.fetch_add(1, std::memory_order_relaxed);

		gThreadPool->Execute([this, jobPtr = job.release()]()
		{
			std::unique_ptr<IJob> ownedJob(jobPtr);

			m_pendingJobs.fetch_sub(1, std::memory_order_relaxed);
			m_activeJobs.fetch_add(1, std::memory_order_relaxed);

			ownedJob->Execute();

			// Decrement under the lock so a waiter cannot evaluate the predicate between the store and the notify.
			{
				std::lock_guard<std::mutex> lock(m_queueMutex);
				m_activeJobs.fetch_sub(1, std::memory_order_relaxed);
			}

			m_completionCondition.notify_all();
		}, priority);
	}
}

//////////////////////////////////////////////////////////////////////////
void CJobScheduler::WaitForAllJobs()
{
	std::unique_lock<std::mutex> lock(m_queueMutex);

	m_completionCondition.wait(lock, [this]()
	{
		return m_pendingJobs.load(std::memory_order_relaxed) == 0 && m_activeJobs.load(std::memory_order_relaxed) == 0;
	});
}

//////////////////////////////////////////////////////////////////////////
size_t CJobScheduler::GetPendingJobCount() const
{
	return m_pendingJobs.load(std::memory_order_relaxed);
}

//////////////////////////////////////////////////////////////////////////
size_t CJobScheduler::GetActiveJobCount() const
{
	return m_activeJobs.load(std::memory_order_relaxed);
}
} // namespace Tge::Threading
