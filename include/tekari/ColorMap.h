#pragma once

#include <tekari/common.h>

TEKARI_NAMESPACE_BEGIN

class ColorMap
{
public:
	static const vector<pair<const string, pair<const uint8_t*, uint32_t>>> PREDEFINED_MAPS;

public:
    ColorMap(const string& name, const uint8_t* color_map_str, uint32_t color_map_str_size);

    void bind(unsigned int target = 0);
    void unbind(unsigned int target = 0);

    unsigned int id() const { return m_render_id; }
    const string& name() const { return m_name; }

private:
    unsigned int m_render_id;
    string m_name;
};

TEKARI_NAMESPACE_END