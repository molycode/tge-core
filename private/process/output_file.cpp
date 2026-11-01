#include <tge/process/output_file.hpp>
#include "native_output_file.hpp"
#include <tge/assert.hpp>
#include <utility>

#if defined(TGE_PLATFORM_LINUX)
#include <cerrno>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#endif // defined(TGE_PLATFORM_LINUX)

namespace Tge::Process
{
namespace
{
#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
}
#elif defined(TGE_PLATFORM_LINUX)
// Owner-only: what a program prints can echo what it was given, a password among it.
constexpr mode_t FileMode{ 0600 };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ errno, std::system_category() };
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
COutputFile::COutputFile() = default;

//////////////////////////////////////////////////////////////////////////
COutputFile::~COutputFile() = default;

//////////////////////////////////////////////////////////////////////////
std::expected<void, std::error_code> COutputFile::Open(std::filesystem::path const& path)
{
	TGE_ASSERT(m_pFile == nullptr, "Output file opened twice");

	auto pFile{ std::make_unique<SNativeOutputFile>() };
	std::error_code error{};

#if defined(TGE_PLATFORM_WINDOWS)
	// Shared for writing too, so a second start can truncate a file an earlier child still writes, as on Linux.
	pFile->file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);

	if (pFile->file == INVALID_HANDLE_VALUE)
	{
		error = GetLastSystemError();
	}
#elif defined(TGE_PLATFORM_LINUX)
	pFile->descriptor = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, FileMode);

	if (pFile->descriptor < 0)
	{
		error = GetLastSystemError();
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	if (error == std::error_code{})
	{
		m_pFile = std::move(pFile);
	}

	return (error == std::error_code{}) ? std::expected<void, std::error_code>{} : std::unexpected{ error };
}

//////////////////////////////////////////////////////////////////////////
void COutputFile::Close()
{
	if (m_pFile != nullptr)
	{
#if defined(TGE_PLATFORM_WINDOWS)
		CloseHandle(m_pFile->file);
#elif defined(TGE_PLATFORM_LINUX)
		::close(m_pFile->descriptor);
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

		m_pFile.reset();
	}
}
} // namespace Tge::Process
