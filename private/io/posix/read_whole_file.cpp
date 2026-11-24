#include <tge/io/read_whole_file.hpp>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace Tge::IO
{
namespace
{
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
} // namespace

//////////////////////////////////////////////////////////////////////////
// O_NONBLOCK, so a FIFO in the file's place cannot hang the caller; it does not affect a regular file.
std::expected<std::string, std::error_code> ReadWholeFile(std::filesystem::path const& path, size_t maxSize)
{
	std::expected<std::string, std::error_code> result{ std::unexpected{ std::error_code{} } };

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

	return result;
}
} // namespace Tge::IO
