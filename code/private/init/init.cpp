#include <tge/init/init.hpp>
#include <tge/logging/log_system.hpp>
#include <tge/memory/linear_allocator.hpp>

namespace Tge
{
namespace Memory::Internal
{
bool Initialize();
void Terminate();
} // namespace Memory::Internal

namespace Threading::Internal
{
bool Initialize(size_t numThreads);
void Terminate();
} // namespace Threading::Internal

//////////////////////////////////////////////////////////////////////////
bool Initialize(size_t numThreads)
{
	bool success = Memory::Internal::Initialize();

	if (success)
	{
		success = Threading::Internal::Initialize(numThreads);
	}

	if (!success)
	{
		Terminate();
	}

	return success;
}

//////////////////////////////////////////////////////////////////////////
void Terminate()
{
	Threading::Internal::Terminate();
	Memory::Internal::Terminate();
}

//////////////////////////////////////////////////////////////////////////
void Update()
{
	Logging::GetLogSystem().DispatchListeners();
	Memory::gFrameAllocator.Reset();
}
} // namespace Tge
