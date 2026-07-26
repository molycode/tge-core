#include <tge/threading/job_system.hpp>
#include "thread_pool.hpp"
#include "job_scheduler.hpp"
#include <tge/logging/loggers.hpp>
#include <rpmalloc/rpmalloc.h>
#include <thread>

namespace Tge::Threading
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
	rpmalloc_thread_initialize();
}

//////////////////////////////////////////////////////////////////////////
void FinalizeThread()
{
	rpmalloc_thread_finalize();
}

namespace Internal
{
bool Initialize(size_t numThreads)
{
	if (numThreads == 0)
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

void Terminate()
{
	if (gJobScheduler)
	{
		delete gJobScheduler;
		gJobScheduler = nullptr;
	}

	if (gThreadPool)
	{
		delete gThreadPool;
		gThreadPool = nullptr;

		gLog.Info("Terminated");
	}
}
} // namespace Internal
} // namespace Tge::Threading
