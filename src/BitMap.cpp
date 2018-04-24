#include "tekari/BitMap.h"

TEKARI_NAMESPACE_BEGIN

BitMap::BitMap(unsigned int size)
    : m_Size(size)
{
    resize(size);
}

unsigned int BitMap::size() const
{
    return m_Size;
}

void BitMap::resize(unsigned int newSize)
{
    m_Size = newSize;
    unsigned int mapSize = m_Size / BLOCK_SIZE;
    if (m_Size % BLOCK_SIZE != 0)
        ++mapSize;
    m_BitMap.resize(mapSize);
}

void BitMap::resize(unsigned int newSize, bool value)
{
    resize(newSize);
    reset(value);
}

void BitMap::reset(bool value)
{
    memset(m_BitMap.data(), value ? ~0 : 0, sizeof(unsigned int) * m_BitMap.size());
}

bool BitMap::get(unsigned int index) const
{
    if (index >= m_Size)
        throw std::runtime_error("Index out of bounds");

    unsigned int mask = 0x1 << bitIndex(index);
    unsigned int value = m_BitMap[mapIndex(index)] & mask;
    return value >> bitIndex(index);
}

void BitMap::set(unsigned int index, bool value)
{
    if (index >= m_Size)
        throw std::runtime_error("Index out of bounds");

    unsigned int mask = 0x1 << bitIndex(index);
    if (value)
        m_BitMap[mapIndex(index)] |= mask;
    else
        m_BitMap[mapIndex(index)] &= ~mask;
}

TEKARI_NAMESPACE_END