#include <tge/io/file.hpp>

#include <fstream>
#include <filesystem>

namespace Tge::IO
{

//////////////////////////////////////////////////////////////////////////
bool CFile::Exists(std::string_view path)
{
	return std::filesystem::exists(path);
}

//////////////////////////////////////////////////////////////////////////
bool CFile::IsDirectory(std::string_view path)
{
	return std::filesystem::is_directory(path);
}

//////////////////////////////////////////////////////////////////////////
bool CFile::IsFile(std::string_view path)
{
	return std::filesystem::is_regular_file(path);
}

//////////////////////////////////////////////////////////////////////////
bool CFile::ReadAllBytes(std::string_view path, std::vector<uint8_t>& outData)
{
	std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		return false;
	}

	std::streamsize const size = file.tellg();
	file.seekg(0, std::ios::beg);

	outData.resize(static_cast<size_t>(size));

	if (!file.read(reinterpret_cast<char*>(outData.data()), size))
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CFile::ReadAllText(std::string_view path, std::string& outText)
{
	std::ifstream file{std::string(path)};

	if (!file.is_open())
	{
		return false;
	}

	file.seekg(0, std::ios::end);
	size_t const size = file.tellg();
	file.seekg(0, std::ios::beg);

	outText.resize(size);

	if (!file.read(outText.data(), static_cast<std::streamsize>(size)))
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CFile::WriteAllBytes(std::string_view path, void const* data, size_t size)
{
	std::ofstream file(std::string(path), std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	if (!file.write(static_cast<char const*>(data), static_cast<std::streamsize>(size)))
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CFile::WriteAllText(std::string_view path, std::string_view text)
{
	std::ofstream file{std::string(path)};

	if (!file.is_open())
	{
		return false;
	}

	if (!file.write(text.data(), static_cast<std::streamsize>(text.size())))
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// A path that was already absent is the caller's desired state, so it reports success; only a filesystem
// error does not.
bool CFile::Delete(std::string_view path)
{
	std::error_code ec;

	std::filesystem::remove(path, ec);

	return !ec;
}

//////////////////////////////////////////////////////////////////////////
size_t CFile::GetFileSize(std::string_view path)
{
	std::error_code ec;
	auto const size = std::filesystem::file_size(path, ec);

	if (ec)
	{
		return 0;
	}

	return static_cast<size_t>(size);
}

//////////////////////////////////////////////////////////////////////////
uint64_t CFile::GetLastWriteTime(std::string_view path)
{
	std::error_code ec;
	auto const ftime = std::filesystem::last_write_time(path, ec);

	if (ec)
	{
		return 0;
	}

	auto const sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
	);

	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count());
}

} // namespace Tge::IO
