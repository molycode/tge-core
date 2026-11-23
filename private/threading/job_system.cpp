#include <tge/threading/job_system.hpp>
#include "thread_pool.hpp"
#include "job_scheduler.hpp"
#include "thread_support.hpp"
#include <tge/logging/loggers.hpp>
#include <algorithm>
#include <chrono>

namespace Tge
{
namespace Memory::Internal
{
void InitializeThread();
void FinalizeThread();
} // namespace Memory::Internal

namespace Threading
{
//////////////////////////////////////////////////////////////////////////
size_t GetNumThreads()
{
	size_t numThreads = 0;

	if (gThreadPool)
	{
		numThreads = gThreadPool->GetNumThreads();
	}

	return numThreads;
}

//////////////////////////////////////////////////////////////////////////
void InitializeThread()
{
	Memory::Internal::InitializeThread();
}

//////////////////////////////////////////////////////////////////////////
void FinalizeThread()
{
	Memory::Internal::FinalizeThread();
}

namespace Internal
{
bool Initialize(size_t numThreads)
{
	size_t const numRequested{ (numThreads == AutoThreadCount) ? GetAutoWorkerThreads() : numThreads };
	size_t const numWorkers{ std::min(numRequested, GetMaxWorkerThreads()) };

	if (numWorkers < numRequested)
	{
		gLog.Warning("{} workers asked for, but this build runs no threads; every job runs on the main thread", numRequested);
	}

	gThreadPool = new CThreadPool(numWorkers);
	gJobScheduler = new CJobScheduler();

	gLog.Info("Initialized ({} workers)", numWorkers);

	return true;
}

// A frame's share of the work no worker can take; at least one item still runs per frame, so work always advances.
constexpr std::chrono::milliseconds MainThreadWorkBudget{ 4 };

void RunMainThreadWork()
{
	if ((gThreadPool != nullptr) && (gThreadPool->GetNumThreads() == 0u))
	{
		gThreadPool->RunQueued(std::chrono::steady_clock::now() + MainThreadWorkBudget);
	}
}

// The pool goes first: until its workers are joined, they can still be inside the scheduler's job wrapper.
void Terminate()
{
	if (gThreadPool)
	{
		delete gThreadPool;
		gThreadPool = nullptr;

		gLog.Info("Terminated");
	}

	if (gJobScheduler)
	{
		delete gJobScheduler;
		gJobScheduler = nullptr;
	}
}
} // namespace Internal
} // namespace Threading
} // namespace Tge
