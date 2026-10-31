#include <tge/io/write_file_atomically.hpp>
#include <tge/platform.hpp>

#if defined(TGE_PLATFORM_WINDOWS)
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include <vector>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#elif defined(TGE_PLATFORM_LINUX)
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

namespace Tge::IO
{
namespace
{
//////////////////////////////////////////////////////////////////////////
// A link a dotfile manager put in the file's place survives, since the rename replaces its target instead.
std::filesystem::path ResolveLink(std::filesystem::path const& path)
{
	std::error_code error{};
	std::filesystem::path const target{ std::filesystem::canonical(path, error) };

	return (error == std::error_code{}) ? target : path;
}

#if defined(TGE_PLATFORM_WINDOWS)
constexpr size_t NumTemporaryNameTries{ 100 };
constexpr std::wstring_view TemporaryNameCharacters{ L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ static_cast<int>(GetLastError()), std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
std::wstring MakeTemporaryPath(std::filesystem::path const& target, std::random_device& random)
{
	std::wstring path{ target.native() + L"." };
	std::uniform_int_distribution<size_t> pick{ 0, TemporaryNameCharacters.size() - 1 };

	for (size_t index{ 0 }; index < 6; ++index)
	{
		path += TemporaryNameCharacters[pick(random)];
	}

	return path;
}

//////////////////////////////////////////////////////////////////////////
// Shared for reading only: nothing else may delete or rename it before it takes the target's place.
HANDLE CreateTemporaryFile(std::filesystem::path const& target, std::error_code& error)
{
	std::random_device random{};
	HANDLE file{ INVALID_HANDLE_VALUE };
	size_t numTries{ 0 };

	error = std::make_error_code(std::errc::file_exists);

	while (file == INVALID_HANDLE_VALUE && error == std::errc::file_exists && numTries < NumTemporaryNameTries)
	{
		file = CreateFileW(MakeTemporaryPath(target, random).c_str(), GENERIC_WRITE | DELETE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		error = (file != INVALID_HANDLE_VALUE) ? std::error_code{} : GetLastSystemError();
		++numTries;
	}

	return file;
}

//////////////////////////////////////////////////////////////////////////
std::error_code WriteAll(HANDLE file, std::string_view content)
{
	std::error_code error{};
	size_t numWritten{ 0 };

	while (error == std::error_code{} && numWritten < content.size())
	{
		DWORD const wanted{ static_cast<DWORD>(std::min<size_t>(content.size() - numWritten, std::numeric_limits<DWORD>::max())) };
		DWORD count{ 0 };

		if (WriteFile(file, content.data() + numWritten, wanted, &count, nullptr) == 0)
		{
			error = GetLastSystemError();
		}
		else if (count == 0)
		{
			error = std::make_error_code(std::errc::io_error);
		}
		else
		{
			numWritten += count;
		}
	}

	return error;
}

//////////////////////////////////////////////////////////////////////////
// Through the open handle, with POSIX semantics: the target is replaced even while a scanner or indexer holds it
// open with delete sharing, which a rename by name refuses. File systems without the Ex form (FAT, SMB) get the classic one.
std::error_code RenameOver(HANDLE file, std::filesystem::path const& target)
{
	std::wstring const& name{ target.native() };
	size_t const nameBytes{ name.size() * sizeof(WCHAR) };
	std::vector<std::byte> buffer(offsetof(FILE_RENAME_INFO, FileName) + nameBytes + sizeof(WCHAR));
	FILE_RENAME_INFO* const pInfo{ reinterpret_cast<FILE_RENAME_INFO*>(buffer.data()) };

	pInfo->Flags = FILE_RENAME_FLAG_REPLACE_IF_EXISTS | FILE_RENAME_FLAG_POSIX_SEMANTICS;
	pInfo->RootDirectory = nullptr;
	pInfo->FileNameLength = static_cast<DWORD>(nameBytes);
	std::memcpy(buffer.data() + offsetof(FILE_RENAME_INFO, FileName), name.c_str(), nameBytes);

	bool isRenamed{ SetFileInformationByHandle(file, FileRenameInfoEx, pInfo, static_cast<DWORD>(buffer.size())) != 0 };
	DWORD const exError{ isRenamed ? DWORD{ ERROR_SUCCESS } : GetLastError() };

	if (!isRenamed && (exError == ERROR_INVALID_PARAMETER || exError == ERROR_NOT_SUPPORTED || exError == ERROR_INVALID_FUNCTION))
	{
		pInfo->Flags = 0;
		pInfo->ReplaceIfExists = TRUE;
		isRenamed = SetFileInformationByHandle(file, FileRenameInfo, pInfo, static_cast<DWORD>(buffer.size())) != 0;
	}

	return isRenamed ? std::error_code{} : GetLastSystemError();
}

//////////////////////////////////////////////////////////////////////////
void DeleteOpenFile(HANDLE file)
{
	FILE_DISPOSITION_INFO_EX disposition{};

	disposition.Flags = FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS;

	if (SetFileInformationByHandle(file, FileDispositionInfoEx, &disposition, sizeof(disposition)) == 0)
	{
		FILE_DISPOSITION_INFO classic{};

		classic.DeleteFile = TRUE;
		SetFileInformationByHandle(file, FileDispositionInfo, &classic, sizeof(classic));
	}
}
#elif defined(TGE_PLATFORM_LINUX)
constexpr std::string_view TemporarySuffix{ ".XXXXXX" };
constexpr mode_t NewFileMode{ 0644 };
constexpr mode_t PermissionBits{ 07777 };

//////////////////////////////////////////////////////////////////////////
std::error_code GetLastSystemError()
{
	return std::error_code{ errno, std::system_category() };
}

//////////////////////////////////////////////////////////////////////////
std::error_code WriteAll(int file, std::string_view content)
{
	std::error_code error{};
	size_t numWritten{ 0 };

	while (error == std::error_code{} && numWritten < content.size())
	{
		ssize_t const count{ ::write(file, content.data() + numWritten, content.size() - numWritten) };

		if (count > 0)
		{
			numWritten += static_cast<size_t>(count);
		}
		else if (count == 0)
		{
			error = std::make_error_code(std::errc::io_error);
		}
		else if (errno != EINTR)
		{
			error = GetLastSystemError();
		}
	}

	return error;
}

//////////////////////////////////////////////////////////////////////////
// An existing file keeps its permissions: it may hold a password.
mode_t GetFileMode(std::filesystem::path const& path)
{
	struct stat status{};

	return (::stat(path.c_str(), &status) == 0) ? (status.st_mode & PermissionBits) : NewFileMode;
}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)
} // namespace

//////////////////////////////////////////////////////////////////////////
// Written beside the file, flushed, then renamed over it, so neither a crash nor a second writer can tear it.
std::expected<void, std::error_code> WriteFileAtomically(std::filesystem::path const& path, std::string_view content)
{
	std::error_code error{};

#if defined(TGE_PLATFORM_WINDOWS)
	std::filesystem::path const resolved{ ResolveLink(path) };
	std::filesystem::path const target{ std::filesystem::absolute(resolved, error) };
	HANDLE const file{ (error == std::error_code{}) ? CreateTemporaryFile(target, error) : INVALID_HANDLE_VALUE };

	if (file != INVALID_HANDLE_VALUE)
	{
		error = WriteAll(file, content);

		if (error == std::error_code{} && FlushFileBuffers(file) == 0)
		{
			error = GetLastSystemError();
		}

		if (error == std::error_code{})
		{
			error = RenameOver(file, target);
		}

		if (error != std::error_code{})
		{
			DeleteOpenFile(file);
		}

		CloseHandle(file);
	}
#elif defined(TGE_PLATFORM_LINUX)
	std::filesystem::path const target{ ResolveLink(path) };
	std::string temporaryPath{ target.string() + std::string{ TemporarySuffix } };
	int const file{ ::mkostemp(temporaryPath.data(), O_CLOEXEC) };

	if (file >= 0)
	{
		if (::fchmod(file, GetFileMode(target)) != 0)
		{
			error = GetLastSystemError();
		}
		else if (error = WriteAll(file, content); error == std::error_code{} && ::fsync(file) != 0)
		{
			error = GetLastSystemError();
		}

		if (::close(file) != 0 && error == std::error_code{})
		{
			error = GetLastSystemError();
		}

		if (error == std::error_code{})
		{
			std::filesystem::rename(temporaryPath, target, error);
		}

		if (error != std::error_code{})
		{
			std::error_code ignored{};

			std::filesystem::remove(temporaryPath, ignored);
		}
	}
	else
	{
		error = GetLastSystemError();
	}
#else
#error "Unsupported platform"
#endif // defined(TGE_PLATFORM_WINDOWS)

	return (error == std::error_code{}) ? std::expected<void, std::error_code>{} : std::unexpected{ error };
}
} // namespace Tge::IO
