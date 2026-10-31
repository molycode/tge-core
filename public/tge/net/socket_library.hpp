#pragma once

#include <tge/non_copyable.hpp>
#include <expected>
#include <system_error>

namespace Tge::Net
{
class CSocketLibrary final : private SNoCopyNoMove
{
public:

	CSocketLibrary() = default;
	~CSocketLibrary() = default;

	std::expected<void, std::error_code> Initialize();
	void Terminate();

private:

	bool m_isStarted{ false };
};
} // namespace Tge::Net
