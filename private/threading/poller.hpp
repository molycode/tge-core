#pragma once

#include <tge/non_copyable.hpp>
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

	bool Add(int descriptor, uint64_t key);
	void Remove(int descriptor);
	// Any thread: ends the current Wait, or the next one.
	void Wake();
	// Fills readyKeys with the keys of the readable descriptors; false once waiting itself fails.
	bool Wait(std::optional<std::chrono::milliseconds> timeout, std::vector<uint64_t>& readyKeys);

private:

	int m_pollDescriptor{ -1 };
	int m_wakeDescriptor{ -1 };
};
} // namespace Tge::Threading
