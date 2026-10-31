#pragma once

#include <tge/io/directory_changes.hpp>
#include <tge/io/watch_scope.hpp>
#include <tge/non_copyable.hpp>
#include <tge/threading/waitable.hpp>
#include <expected>
#include <filesystem>
#include <memory>
#include <system_error>

namespace Tge::IO
{
struct SNativeDirectoryWatch;

class CDirectoryWatch final : private SNoCopyNoMove
{
public:

	CDirectoryWatch();
	~CDirectoryWatch();

	std::expected<void, std::error_code> Initialize(std::filesystem::path const& folder, EWatchScope scope);
	void Terminate();

	std::expected<SDirectoryChanges, std::error_code> ReadChanges();
	Threading::SWaitable GetWaitable() const;

private:

	std::unique_ptr<SNativeDirectoryWatch> m_pWatch{};
};
} // namespace Tge::IO
