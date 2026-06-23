#pragma once

#include <tge/threading/job.hpp>
#include <tge/non_copyable.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace Tge::Threading
{
class CJobScheduler final : private SNoCopyNoMove
{
public:

	CJobScheduler() = default;
	~CJobScheduler() = default;

	void SubmitJob(std::unique_ptr<IJob> job, EJobPriority priority);
	void WaitForAllJobs();

	size_t GetPendingJobCount() const;
	size_t GetActiveJobCount() const;

private:

	struct SJobEntry final
	{
		std::unique_ptr<IJob> job;
		EJobPriority priority;

		bool operator<(SJobEntry const& other) const
		{
			return priority > other.priority;
		}
	};

	void ProcessNextJob();

	std::priority_queue<SJobEntry> m_jobQueue;
	std::mutex m_queueMutex;
	std::condition_variable m_completionCondition;
	std::atomic<size_t> m_pendingJobs{0};
	std::atomic<size_t> m_activeJobs{0};
};

extern CJobScheduler* gJobScheduler;
} // namespace Tge::Threading
