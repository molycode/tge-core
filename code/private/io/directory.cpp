#include <tge/io/directory.hpp>
#include <filesystem>

#ifdef __linux__
#include <unistd.h>
#include <linux/limits.h>
#endif

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
bool CDirectory::Create(std::string_view path)
{
	std::error_code ec;
	return std::filesystem::create_directories(path, ec);
}

//////////////////////////////////////////////////////////////////////////
bool CDirectory::Delete(std::string_view path, bool recursive)
{
	std::error_code ec;

	if (recursive)
	{
		std::filesystem::remove_all(path, ec);
	}
	else
	{
		std::filesystem::remove(path, ec);
	}

	return !ec;
}

//////////////////////////////////////////////////////////////////////////
bool CDirectory::GetFiles(std::string_view path, std::vector<std::string>& outFiles, bool recursive)
{
	std::error_code ec;

	if (recursive)
	{
		for (auto const& entry : std::filesystem::recursive_directory_iterator(path, ec))
		{
			if (entry.is_regular_file())
			{
				outFiles.push_back(entry.path().string());
			}
		}
	}
	else
	{
		for (auto const& entry : std::filesystem::directory_iterator(path, ec))
		{
			if (entry.is_regular_file())
			{
				outFiles.push_back(entry.path().string());
			}
		}
	}

	return !ec;
}

//////////////////////////////////////////////////////////////////////////
bool CDirectory::GetDirectories(std::string_view path, std::vector<std::string>& outDirs, bool recursive)
{
	std::error_code ec;

	if (recursive)
	{
		for (auto const& entry : std::filesystem::recursive_directory_iterator(path, ec))
		{
			if (entry.is_directory())
			{
				outDirs.push_back(entry.path().string());
			}
		}
	}
	else
	{
		for (auto const& entry : std::filesystem::directory_iterator(path, ec))
		{
			if (entry.is_directory())
			{
				outDirs.push_back(entry.path().string());
			}
		}
	}

	return !ec;
}

//////////////////////////////////////////////////////////////////////////
std::string CDirectory::GetCurrentWorkingDirectory()
{
	std::error_code ec;
	auto const cwd = std::filesystem::current_path(ec);

	if (ec)
	{
		return "";
	}

	return cwd.string();
}

//////////////////////////////////////////////////////////////////////////
std::string CDirectory::GetExecutableDirectory()
{
#ifdef __linux__
	char buffer[PATH_MAX];
	ssize_t const len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

	if (len != -1)
	{
		buffer[len] = '\0';
		std::filesystem::path exePath(buffer);
		return exePath.parent_path().string();
	}

	return "";
#else
	#error "Unsupported platform: GetExecutableDirectory() not implemented"
#endif
}
} // namespace Tge::IO
