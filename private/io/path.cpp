#include <tge/io/path.hpp>

#include <filesystem>

namespace Tge::IO
{

//////////////////////////////////////////////////////////////////////////
std::string CPath::Join(std::string_view a, std::string_view b)
{
	std::filesystem::path pathA(a);
	std::filesystem::path pathB(b);
	return (pathA / pathB).string();
}

//////////////////////////////////////////////////////////////////////////
std::string CPath::GetDirectory(std::string_view path)
{
	return std::filesystem::path(path).parent_path().string();
}

//////////////////////////////////////////////////////////////////////////
std::string CPath::GetFilename(std::string_view path)
{
	return std::filesystem::path(path).filename().string();
}

//////////////////////////////////////////////////////////////////////////
std::string CPath::GetExtension(std::string_view path)
{
	return std::filesystem::path(path).extension().string();
}

//////////////////////////////////////////////////////////////////////////
std::string CPath::GetFilenameWithoutExtension(std::string_view path)
{
	return std::filesystem::path(path).stem().string();
}

//////////////////////////////////////////////////////////////////////////
std::string CPath::Normalize(std::string_view path)
{
	return std::filesystem::path(path).lexically_normal().string();
}

//////////////////////////////////////////////////////////////////////////
std::string CPath::GetAbsolutePath(std::string_view relativePath)
{
	std::error_code ec;
	auto const absPath = std::filesystem::absolute(relativePath, ec);

	if (ec)
	{
		return std::string(relativePath);
	}

	return absPath.string();
}

//////////////////////////////////////////////////////////////////////////
bool CPath::IsAbsolute(std::string_view path)
{
	return std::filesystem::path(path).is_absolute();
}

//////////////////////////////////////////////////////////////////////////
bool CPath::IsRelative(std::string_view path)
{
	return std::filesystem::path(path).is_relative();
}

} // namespace Tge::IO
