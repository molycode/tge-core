#pragma once

#include <tge/non_copyable.hpp>
#include <tge/process/exit_status.hpp>
#include <tge/process/process_spec.hpp>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <system_error>

namespace Tge::Process
{
struct SNativeChildProcess;

class CChildProcess final : private SNoCopyNoMove
{
public:

	CChildProcess();
	~CChildProcess();

	std::expected<void, std::error_code> Start(SProcessSpec const& spec);
	void Terminate();

	std::optional<std::expected<SExitStatus, std::error_code>> TryGetExit();
	uint32_t GetId() const;

private:

	std::unique_ptr<SNativeChildProcess> m_pProcess{};
};
} // namespace Tge::Process
