#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace Tge::IO
{
class CFile final
{
public:

	static bool Exists(std::string_view path);
	static bool IsDirectory(std::string_view path);
	static bool IsFile(std::string_view path);

	static bool ReadAllBytes(std::string_view path, std::vector<uint8_t>& outData);
	static bool ReadAllText(std::string_view path, std::string& outText);

	static bool WriteAllBytes(std::string_view path, void const* data, size_t size);
	static bool WriteAllText(std::string_view path, std::string_view text);

	static bool Delete(std::string_view path);

	static size_t GetFileSize(std::string_view path);
	static uint64_t GetLastWriteTime(std::string_view path);
};
} // namespace Tge::IO
