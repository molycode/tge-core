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

namespace IO::Internal
{
bool Initialize();
void Terminate();
} // namespace IO::Internal

//////////////////////////////////////////////////////////////////////////
bool Initialize(size_t numThreads)
{
	bool success = Memory::Internal::Initialize();

	if (success)
	{
		success = Threading::Internal::Initialize(numThreads);
	}

	if (success)
	{
		success = IO::Internal::Initialize();
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
	IO::Internal::Terminate();
	Threading::Internal::Terminate();
	Memory::Internal::Terminate();
}

//////////////////////////////////////////////////////////////////////////
void Update()
{
	Logging::GetLogSystem().DispatchListeners();
	Memory::g_frameAllocator.Reset();
}
} // namespace Tge
