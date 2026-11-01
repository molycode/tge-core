#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Tge::Process
{
class COutputFile;

struct SProcessSpec final
{
	std::filesystem::path program{};
	std::vector<std::string> arguments{};
	std::filesystem::path workingDir{};
	COutputFile const* pOutput{ nullptr };
};
} // namespace Tge::Process
