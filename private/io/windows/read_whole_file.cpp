#include <tge/io/read_whole_file.hpp>
#include <algorithm>
#include <limits>
#include <utility>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>

namespace Tge::IO
{
namespace
{
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
} // namespace

//////////////////////////////////////////////////////////////////////////
// Shared for deletion, else a reader would make WriteFileAtomically fail.
std::expected<std::string, std::error_code> ReadWholeFile(std::filesystem::path const& path, size_t maxSize)
{
	std::expected<std::string, std::error_code> result{ std::unexpected{ std::error_code{} } };

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

	return result;
}
} // namespace Tge::IO
