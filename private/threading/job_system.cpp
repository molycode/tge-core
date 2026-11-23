#include <tge/threading/job_system.hpp>
#include "thread_pool.hpp"
#include "job_scheduler.hpp"
#include <tge/logging/loggers.hpp>
#include <thread>

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
	if (numThreads == AutoThreadCount)
	{
		numThreads = std::thread::hardware_concurrency();

		if (numThreads == 0)
		{
			numThreads = 4;
		}
	}

	gThreadPool = new CThreadPool(numThreads);
	gJobScheduler = new CJobScheduler();

	gLog.Info("Initialized ({} workers)", numThreads);

	return true;
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
