#include <tge/io/stream.hpp>
#include <algorithm>
#include <cstring>

namespace Tge::IO
{
//////////////////////////////////////////////////////////////////////////
CBinaryReader::CBinaryReader(std::vector<uint8_t> const& data)
	: m_data(data)
{
}

//////////////////////////////////////////////////////////////////////////
void CBinaryReader::ReadBytes(void* dest, size_t size)
{
	if (m_position + size <= m_data.size())
	{
		std::memcpy(dest, m_data.data() + m_position, size);
		m_position += size;
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBinaryReader::IsEOF() const
{
	return m_position >= m_data.size();
}

//////////////////////////////////////////////////////////////////////////
size_t CBinaryReader::GetPosition() const
{
	return m_position;
}

//////////////////////////////////////////////////////////////////////////
void CBinaryReader::Seek(size_t offset)
{
	m_position = std::min(offset, m_data.size());
}

//////////////////////////////////////////////////////////////////////////
void CBinaryWriter::WriteBytes(void const* src, size_t size)
{
	size_t const oldSize = m_data.size();
	m_data.resize(oldSize + size);
	std::memcpy(m_data.data() + oldSize, src, size);
}

//////////////////////////////////////////////////////////////////////////
std::vector<uint8_t> const& CBinaryWriter::GetData() const
{
	return m_data;
}
} // namespace Tge::IO
