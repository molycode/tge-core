#include <tge/threading/job_system.hpp>
#include "thread_pool.hpp"
#include "job_scheduler.hpp"
#include <rpmalloc/rpmalloc.h>
#include <thread>

namespace Tge::Threading
{
//////////////////////////////////////////////////////////////////////////
size_t GetNumThreads()
{
	size_t numThreads = 0;

	if (g_threadPool)
	{
		numThreads = g_threadPool->GetNumThreads();
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

	g_threadPool = new CThreadPool(numThreads);
	g_jobScheduler = new CJobScheduler();

	return true;
}

void Terminate()
{
	if (g_jobScheduler)
	{
		delete g_jobScheduler;
		g_jobScheduler = nullptr;
	}

	if (g_threadPool)
	{
		delete g_threadPool;
		g_threadPool = nullptr;
	}
}
} // namespace Internal
} // namespace Tge::Threading
