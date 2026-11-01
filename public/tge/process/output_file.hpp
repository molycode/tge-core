#pragma once

#include <tge/non_copyable.hpp>
#include <expected>
#include <filesystem>
#include <memory>
#include <system_error>

namespace Tge::Process
{
struct SNativeOutputFile;

class COutputFile final : private SNoCopyNoMove
{
public:

	COutputFile();
	~COutputFile();

	std::expected<void, std::error_code> Open(std::filesystem::path const& path);
	void Close();

private:

	friend class CChildProcess;

	std::unique_ptr<SNativeOutputFile> m_pFile{};
};
} // namespace Tge::Process
