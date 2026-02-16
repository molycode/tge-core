#include <tge/memory/default_allocator.hpp>
#include <tge/memory/linear_allocator.hpp>
#include <rpmalloc/rpmalloc.h>
#include "../loggers.hpp"

namespace Tge::Memory::Internal
{
//////////////////////////////////////////////////////////////////////////
bool Initialize()
{
	// Initialize rpmalloc
	int const result = rpmalloc_initialize(nullptr);

	if (result != 0)
	{
		gLog.Error("Failed to initialize rpmalloc");
		return false;
	}

	// Initialize rpmalloc for main thread
	rpmalloc_thread_initialize();

	gLog.Info("Initialized (rpmalloc + frame allocator 16 MiB)");

	return true;
}

//////////////////////////////////////////////////////////////////////////
void Terminate()
{
	// Finalize rpmalloc for main thread
	rpmalloc_thread_finalize();

	// Finalize rpmalloc globally
	rpmalloc_finalize();

	gLog.Info("Terminated");
}
} // namespace Tge::Memory::Internal
