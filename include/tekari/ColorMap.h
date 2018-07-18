#pragma once

#include <tekari/common.h>

TEKARI_NAMESPACE_BEGIN

class ColorMap
{
public:
    static const vector<std::pair<const string, const string>> PREDEFINED_MAPS;

public:
    ColorMap(const string& name, const string& file_path);

    void bind(unsigned int target = 0);
    void unbind(unsigned int target = 0);

    unsigned int id() const { return m_render_id; }
    const string& name() const { return m_name; }

private:
    unsigned int m_render_id;
    string m_name;
};

TEKARI_NAMESPACE_END