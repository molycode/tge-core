#include <tge/io/write_file_atomically.hpp>
#include "io/posix/flush_file.hpp"
#include "io/resolve_link.hpp"
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace Tge::IO
{
namespace
{
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
} // namespace

//////////////////////////////////////////////////////////////////////////
// Written beside the file, flushed, then renamed over it, so neither a crash nor a second writer can tear it.
std::expected<void, std::error_code> WriteFileAtomically(std::filesystem::path const& path, std::string_view content)
{
	std::error_code error{};
	std::filesystem::path const target{ ResolveLink(path) };
	std::string temporaryPath{ target.string() + std::string{ TemporarySuffix } };
	int const file{ ::mkostemp(temporaryPath.data(), O_CLOEXEC) };

	if (file >= 0)
	{
		if (::fchmod(file, GetFileMode(target)) != 0)
		{
			error = GetLastSystemError();
		}
		else if (error = WriteAll(file, content); error == std::error_code{})
		{
			error = FlushFile(file);
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

	return (error == std::error_code{}) ? std::expected<void, std::error_code>{} : std::unexpected{ error };
}
} // namespace Tge::IO
