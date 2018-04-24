#pragma once

#include <vector>
#include <iostream>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class BitMap
{
public:
    BitMap(unsigned int size = 0);

    unsigned int size() const;
    void resize(unsigned int newSize);
    void resize(unsigned int newSize, bool value);

    void reset(bool value = false);

    bool get(unsigned int index) const;
    void set(unsigned int index, bool value);

private:
    inline unsigned int mapIndex(unsigned int index) const { return index / BLOCK_SIZE; }
    inline unsigned int bitIndex(unsigned int index) const { return index % BLOCK_SIZE; }

private:
    static const unsigned int BLOCK_SIZE = 32;
    std::vector<unsigned int> m_BitMap;
    unsigned int m_Size;
};

inline std::ostream& operator<<(std::ostream& os, const BitMap& bitmap)
{
    for (unsigned int i = 0; i < bitmap.size(); ++i)
    {
        os << bitmap.get(i) << " ";
    }
    os << "\n";
    return os;
}

TEKARI_NAMESPACE_END