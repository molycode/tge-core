#include <tge/threading/job_group.hpp>
#include "job_scheduler.hpp"

namespace Tge::Threading
{
CJobGroup gDefaultJobGroup;

//////////////////////////////////////////////////////////////////////////
void CJobGroup::SubmitJob(std::unique_ptr<IJob> job, EJobPriority priority)
{
	if (job && gJobScheduler)
	{
		m_activeJobs.fetch_add(1, std::memory_order_relaxed);

	class CWrappedJob final : public IJob
	{
	public:

		explicit CWrappedJob(std::unique_ptr<IJob> innerJob, CJobGroup* group)
			: m_innerJob(std::move(innerJob))
			, m_group(group)
		{
		}

		void Execute() override
		{
			m_innerJob->Execute();

			m_group->m_activeJobs.fetch_sub(1, std::memory_order_relaxed);
			m_group->m_completionCondition.notify_all();
		}

	private:

		std::unique_ptr<IJob> m_innerJob;
		CJobGroup* m_group;
	};

		std::unique_ptr<IJob> wrappedJob(new CWrappedJob(std::move(job), this));
		gJobScheduler->SubmitJob(std::move(wrappedJob), priority);
	}
}

//////////////////////////////////////////////////////////////////////////
void CJobGroup::Wait()
{
	std::unique_lock<std::mutex> lock(m_waitMutex);

	m_completionCondition.wait(lock, [this]()
	{
		return m_activeJobs.load() == 0;
	});
}

//////////////////////////////////////////////////////////////////////////
bool CJobGroup::IsComplete() const
{
	return m_activeJobs.load(std::memory_order_relaxed) == 0;
}

//////////////////////////////////////////////////////////////////////////
size_t CJobGroup::GetActiveCount() const
{
	return m_activeJobs.load(std::memory_order_relaxed);
}
} // namespace Tge::Threading
