#include <tge/io/read_whole_file.hpp>
#include <tge/platform.hpp>
#include <utility>

#if defined(TGE_PLATFORM_WINDOWS)
#include <algorithm>
#include <limits>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::IO
{
namespace
{
#if defined(TGE_PLATFORM_WINDOWS)
//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<std::string, std::error_code> ReadOpenFile(HANDLE file, size_t maxSize)
{
	std::expected<std::string, std::error_code> result{ std::unexpected{ std::error_code{} } };
	BY_HANDLE_FILE_INFORMATION information{};
	LARGE_INTEGER size{};

	if (GetFileType(file) != FILE_TYPE_DISK)
	{
		result = std::unexpected{ std::make_error_code(std::errc::invalid_argument) };
	}
	else if (GetFileInformationByHandle(file, &information) == 0 || GetFileSizeEx(file, &size) == 0)
	{
		result = std::unexpected{ GetLastSystemError() };
	}
	else if ((information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
	{
		result = std::unexpected{ std::make_error_code(std::errc::is_a_directory) };
	}
	else if (static_cast<unsigned long long>(size.QuadPart) > maxSize)
	{
		result = std::unexpected{ std::make_error_code(std::errc::file_too_large) };
	}
	else
	{
		std::string text{};
		std::error_code readError{};
		size_t numRead{ 0 };
		bool isDone{ size.QuadPart == 0 };

		text.resize(static_cast<size_t>(size.QuadPart));

		while (!isDone)
		{
			DWORD const wanted{ static_cast<DWORD>(std::min<size_t>(text.size() - numRead, std::numeric_limits<DWORD>::max())) };
			DWORD count{ 0 };

			if (::ReadFile(file, text.data() + numRead, wanted, &count, nullptr) == 0)
			{
				readError = GetLastSystemError();
				isDone = true;
			}
			else if (count == 0)
			{
				text.resize(numRead);
				isDone = true;
			}
			else
			{
				numRead += count;
				isDone = numRead == text.size();
			}
		}

		if (readError == std::error_code{})
		{
			result = std::move(text);
		}
		else
		{
			result = std::unexpected{ readError };
		}
	}

	return result;
}
#elif defined(TGE_PLATFORM_LINUX)
//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ errno, std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
std::expected<std::string, std::error_code> ReadOpenFile(int file, size_t maxSize)
{
	std::expected<std::string, std::error_code> result{ std::unexpected{ std::error_code{} } };
	struct stat status{};

	if (::fstat(file, &status) != 0)
	{
		result = std::unexpected{ GetLastSystemError() };
	}
	else if (S_ISDIR(status.st_mode))
	{
		result = std::unexpected{ std::make_error_code(std::errc::is_a_directory) };
	}
	else if (!S_ISREG(status.st_mode))
	{
		result = std::unexpected{ std::make_error_code(std::errc::invalid_argument) };
	}
	else if (static_cast<size_t>(status.st_size) > maxSize)
	{
		result = std::unexpected{ std::make_error_code(std::errc::file_too_large) };
	}
	else
	{
		std::string text{};
		std::error_code readError{};
		size_t numRead{ 0 };
		bool isDone{ status.st_size == 0 };

		text.resize(static_cast<size_t>(status.st_size));

		while (!isDone)
		{
			ssize_t const count{ ::read(file, text.data() + numRead, text.size() - numRead) };

			if (count > 0)
			{
				numRead += static_cast<size_t>(count);
				isDone = numRead == text.size();
			}
			else if (count == 0)
			{
				text.resize(numRead);
				isDone = true;
			}
			else if (errno != EINTR)
			{
				readError = GetLastSystemError();
				isDone = true;
			}
		}

		if (readError == std::error_code{})
		{
			result = std::move(text);
		}
		else
		{
			result = std::unexpected{ readError };
		}
	}

	return result;
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
// Shared for deletion on Windows, else a reader would make WriteFileAtomically fail. O_NONBLOCK on Linux, so a FIFO in
// the file's place cannot hang the caller; it does not affect a regular file.
std::expected<std::string, std::error_code> ReadWholeFile(std::filesystem::path const& path, size_t maxSize)
{
	std::expected<std::string, std::error_code> result{ std::unexpected{ std::error_code{} } };

#if defined(TGE_PLATFORM_WINDOWS)
	HANDLE const file{ CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN, nullptr) };

	if (file != INVALID_HANDLE_VALUE)
	{
		result = ReadOpenFile(file, maxSize);
		CloseHandle(file);
	}
	else
	{
		result = std::unexpected{ GetLastSystemError() };
	}
#elif defined(TGE_PLATFORM_LINUX)
	int const file{ ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK) };

	if (file >= 0)
	{
		result = ReadOpenFile(file, maxSize);
		::close(file);
	}
	else
	{
		result = std::unexpected{ GetLastSystemError() };
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return result;
}
} // namespace Tge::IO
