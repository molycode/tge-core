#pragma once

#include <tge/threading/job.hpp>
#include <tge/non_copyable.hpp>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace Tge::Threading
{
// Jobs are ordered by CThreadPool's priority queue. The scheduler keeps no queue of its own: holding one here
// and draining it on every submit meant a job's priority only ever raced against jobs submitted in the same
// instant, so a High job still landed behind everything already queued in the pool.
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

	std::mutex m_queueMutex;
	std::condition_variable m_completionCondition;
	std::atomic<size_t> m_pendingJobs{0};
	std::atomic<size_t> m_activeJobs{0};
};

extern CJobScheduler* gJobScheduler;
} // namespace Tge::Threading
