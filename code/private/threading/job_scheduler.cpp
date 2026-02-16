#include "job_scheduler.hpp"
#include "thread_pool.hpp"

namespace Tge::Threading
{
CJobScheduler* g_jobScheduler = nullptr;

//////////////////////////////////////////////////////////////////////////
void CJobScheduler::SubmitJob(std::unique_ptr<IJob> job, EJobPriority priority)
{
	if (job)
	{
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_jobQueue.push(SJobEntry{std::move(job), priority});
			m_pendingJobs.fetch_add(1, std::memory_order_relaxed);
		}

		ProcessNextJob();
	}
}

//////////////////////////////////////////////////////////////////////////
void CJobScheduler::WaitForAllJobs()
{
	std::unique_lock<std::mutex> lock(m_queueMutex);

	m_completionCondition.wait(lock, [this]()
	{
		return m_jobQueue.empty() && m_activeJobs.load() == 0;
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

//////////////////////////////////////////////////////////////////////////
void CJobScheduler::ProcessNextJob()
{
	std::unique_ptr<IJob> job;

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);

		if (!m_jobQueue.empty())
		{
			SJobEntry entry = std::move(const_cast<SJobEntry&>(m_jobQueue.top()));
			m_jobQueue.pop();
			job = std::move(entry.job);
			m_pendingJobs.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	if (job && g_threadPool)
	{
		m_activeJobs.fetch_add(1, std::memory_order_relaxed);

		g_threadPool->Execute([this, jobPtr = job.release()]()
		{
			std::unique_ptr<IJob> ownedJob(jobPtr);
			ownedJob->Execute();

			m_activeJobs.fetch_sub(1, std::memory_order_relaxed);
			m_completionCondition.notify_all();
		});
	}
}
} // namespace Tge::Threading
