#pragma once

#include "poller_entry.hpp"
#include <tge/non_copyable.hpp>
#include <tge/platform.hpp>
#include <tge/threading/waitable.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace Tge::Threading
{
// CEventLoop's operating-system side: registered descriptors plus a wake signal any thread can raise.
class CPoller final : private SNoCopyNoMove
{
public:

	CPoller() = default;
	~CPoller() = default;

	bool Initialize();
	void Terminate();

	bool Add(SWaitable waitable, uint64_t key);
	void Remove(SWaitable waitable);
	// Any thread: ends the current Wait, or the next one.
	void Wake();
	// Fills readyKeys with the keys of the readable descriptors; false once waiting itself fails.
	bool Wait(std::optional<std::chrono::milliseconds> timeout, std::vector<uint64_t>& readyKeys);

private:

#if defined(TGE_PLATFORM_LINUX)
	int m_pollDescriptor{ -1 };
	int m_wakeDescriptor{ -1 };
#elif defined(TGE_PLATFORM_WINDOWS)
	void* m_wakeEvent{ nullptr };
	void* m_socketEvent{ nullptr };
	std::vector<SPollerEntry> m_entries{};
#endif // defined(TGE_PLATFORM_LINUX)
};
} // namespace Tge::Threading
