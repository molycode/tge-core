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

	// Inherits cancellation from the job group whose job is running on the constructing thread (if any),
	// so a nested group is cancelled together with its parent — no manual wiring at the bake site.
	CJobGroup();
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

	// Cancellation is sticky/terminal (intended for teardown) — no Reset by design. Cascades to nested groups.
	// Queued jobs are dropped/skipped so Wait() returns promptly; a running job body bails via IsCurrentJobCancelled().
	void Cancel();
	bool IsCancellationRequested() const;

private:

	friend bool IsCurrentJobCancelled();

	// Shared so a dispatched runner can outlive the CJobGroup handle (stack-local groups) without dangling.
	struct SState final : std::enable_shared_from_this<SState>
	{
		// Runs one of this group's queued jobs on the calling thread; false when none queued.
		bool RunOnePending();

		// This group's own flag OR any ancestor's — cancellation propagates down the nesting chain.
		bool IsCancelled() const;

		// Group whose job is currently executing on this thread; lets a nested group find its parent
		// and IsCurrentJobCancelled() find the running group without any handle being passed in.
		static thread_local SState* tCurrent;

		std::atomic<size_t> activeJobs{ 0 };
		std::atomic<bool> cancelled{ false };
		std::shared_ptr<SState> parent;   // group whose job was running when this one was constructed
		std::mutex mutex;
		std::deque<std::unique_ptr<IJob>> pending;
		std::condition_variable completion;
	};

	std::shared_ptr<SState> m_state{ std::make_shared<SState>() };
};

// Callable from inside a running job: true if the job's group (or an ancestor) has been cancelled.
// Lets a long job body bail cooperatively with no cancellation handle threaded in.
bool IsCurrentJobCancelled();

extern CJobGroup gDefaultJobGroup;
} // namespace Tge::Threading
