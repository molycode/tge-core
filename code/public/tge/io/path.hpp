#pragma once

#include <string>
#include <string_view>

namespace Tge::IO
{
class CPath final
{
public:

	static std::string Join(std::string_view a, std::string_view b);
	static std::string GetDirectory(std::string_view path);
	static std::string GetFilename(std::string_view path);
	static std::string GetExtension(std::string_view path);
	static std::string GetFilenameWithoutExtension(std::string_view path);
	static std::string Normalize(std::string_view path);
	static std::string GetAbsolutePath(std::string_view relativePath);
	static bool IsAbsolute(std::string_view path);
	static bool IsRelative(std::string_view path);
};
} // namespace Tge::IO
