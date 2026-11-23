#include <tge/logging/loggers.hpp>

#ifdef TGE_GLOBAL_ALLOCATOR_ENABLED
#include <rpmalloc/rpmalloc.h>
#endif // TGE_GLOBAL_ALLOCATOR_ENABLED

namespace Tge::Memory::Internal
{
//////////////////////////////////////////////////////////////////////////
bool Initialize()
{
	bool success{ true };

#ifdef TGE_GLOBAL_ALLOCATOR_ENABLED
	success = rpmalloc_initialize(nullptr) == 0;

	if (success)
	{
		rpmalloc_thread_initialize();
		gLog.Info("Initialized (rpmalloc)");
	}
	else
	{
		gLog.Error("Failed to initialize rpmalloc");
	}
#else
	gLog.Info("Initialized (C runtime heap)");
#endif // TGE_GLOBAL_ALLOCATOR_ENABLED

	return success;
}

//////////////////////////////////////////////////////////////////////////
void Terminate()
{
#ifdef TGE_GLOBAL_ALLOCATOR_ENABLED
	rpmalloc_thread_finalize();
	rpmalloc_finalize();
#endif // TGE_GLOBAL_ALLOCATOR_ENABLED

	gLog.Info("Terminated");
}

//////////////////////////////////////////////////////////////////////////
void InitializeThread()
{
#ifdef TGE_GLOBAL_ALLOCATOR_ENABLED
	rpmalloc_thread_initialize();
#endif // TGE_GLOBAL_ALLOCATOR_ENABLED
}

//////////////////////////////////////////////////////////////////////////
void FinalizeThread()
{
#ifdef TGE_GLOBAL_ALLOCATOR_ENABLED
	rpmalloc_thread_finalize();
#endif // TGE_GLOBAL_ALLOCATOR_ENABLED
}
} // namespace Tge::Memory::Internal
