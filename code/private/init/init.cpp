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

// Nothing to initialize - the registry is usable from static-init onwards. Only the group sweep needs a hook.
namespace Command::Internal
{
void Terminate();
} // namespace Command::Internal

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
	// Ahead of Memory so the group sweep's deallocation tracking still lands.
	Command::Internal::Terminate();
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
