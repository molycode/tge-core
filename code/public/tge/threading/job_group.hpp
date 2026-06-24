#pragma once

#include "job.hpp"
#include <tge/non_copyable.hpp>
#include <memory>
#include <deque>
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

	// Shared so a dispatched runner can outlive the CJobGroup handle (stack-local groups) without dangling.
	struct SState final
	{
		// Runs one of this group's queued jobs on the calling thread; false when none queued.
		bool RunOnePending();

		std::atomic<size_t> activeJobs{ 0 };
		std::mutex mutex;
		std::deque<std::unique_ptr<IJob>> pending;
		std::condition_variable completion;
	};

	std::shared_ptr<SState> m_state{ std::make_shared<SState>() };
};

extern CJobGroup gDefaultJobGroup;
} // namespace Tge::Threading
