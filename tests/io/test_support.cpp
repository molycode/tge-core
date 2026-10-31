#include "test_support.hpp"
#include <format>
#include <fstream>
#include <random>
#include <sstream>
#include <system_error>

namespace Tge::IO::Tests
{
//////////////////////////////////////////////////////////////////////////
// Empty when no folder could be made.
std::filesystem::path MakeTemporaryFolder(std::string_view prefix)
{
	std::error_code error{};
	std::filesystem::path const base{ std::filesystem::temp_directory_path(error) };
	std::random_device random{};
	std::filesystem::path made{};

	for (int attempt{ 0 }; attempt < 100 && made.empty() && error == std::error_code{}; ++attempt)
	{
		std::filesystem::path const candidate{ base / std::format("{}-{:08x}", prefix, random()) };

		if (std::filesystem::create_directory(candidate, error))
		{
			made = candidate;
		}
	}

	return made;
}

//////////////////////////////////////////////////////////////////////////
void WriteText(std::filesystem::path const& path, std::string_view text)
{
	std::ofstream{ path, std::ios::binary } << text;
}

//////////////////////////////////////////////////////////////////////////
std::string ReadText(std::filesystem::path const& path)
{
	std::ifstream stream{ path, std::ios::binary };
	std::ostringstream text{};

	text << stream.rdbuf();

	return text.str();
}

//////////////////////////////////////////////////////////////////////////
size_t CountEntries(std::filesystem::path const& folder)
{
	std::error_code error{};
	size_t numEntries{ 0 };

	for (std::filesystem::directory_iterator entry{ folder, error }, end{}; error == std::error_code{} && entry != end; entry.increment(error))
	{
		++numEntries;
	}

	return numEntries;
}
} // namespace Tge::IO::Tests
