#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace Tge::IO
{
class CDirectory final
{
public:

	static bool Create(std::string_view path);
	static bool Delete(std::string_view path, bool recursive = false);
	static bool GetFiles(std::string_view path, std::vector<std::string>& outFiles, bool recursive = false);
	static bool GetDirectories(std::string_view path, std::vector<std::string>& outDirs, bool recursive = false);
	static std::string GetCurrentWorkingDirectory();
	static std::string GetExecutableDirectory();
};
} // namespace Tge::IO
