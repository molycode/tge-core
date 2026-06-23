#pragma once

#include "job.hpp"
#include <tge/non_copyable.hpp>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace Tge::Threading
{
class CJobGroup final : private Tge::SNoCopyNoMove
{
public:

	CJobGroup() = default;
	~CJobGroup() = default;

	void SubmitJob(std::unique_ptr<IJob> job, EJobPriority priority = EJobPriority::Normal);

	template<typename Func>
	void SubmitJob(Func&& func, EJobPriority priority = EJobPriority::Normal)
	{
		std::unique_ptr<IJob> job(new CLambdaJob<Func>(std::forward<Func>(func)));
		SubmitJob(std::move(job), priority);
	}

	void Wait();
	bool IsComplete() const;
	size_t GetActiveCount() const;

private:

	std::atomic<size_t> m_activeJobs{0};
	std::mutex m_waitMutex;
	std::condition_variable m_completionCondition;
};

extern CJobGroup gDefaultJobGroup;
} // namespace Tge::Threading
