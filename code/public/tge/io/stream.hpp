#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Tge::IO
{
class CBinaryReader final
{
public:

	explicit CBinaryReader(std::vector<uint8_t> const& data);

	template<typename T>
	T Read();

	void ReadBytes(void* dest, size_t size);
	bool IsEOF() const;
	size_t GetPosition() const;
	void Seek(size_t offset);

private:

	std::vector<uint8_t> const& m_data;
	size_t m_position = 0;
};

class CBinaryWriter final
{
public:

	CBinaryWriter() = default;

	template<typename T>
	void Write(T const& value);

	void WriteBytes(void const* src, size_t size);
	std::vector<uint8_t> const& GetData() const;

private:

	std::vector<uint8_t> m_data;
};

// Template implementations
template<typename T>
T CBinaryReader::Read()
{
	T value;
	ReadBytes(&value, sizeof(T));
	return value;
}

template<typename T>
void CBinaryWriter::Write(T const& value)
{
	WriteBytes(&value, sizeof(T));
}
} // namespace Tge::IO
