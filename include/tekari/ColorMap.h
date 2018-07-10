#pragma once

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class ColorMap
{
public:
    static const std::string FOLDER_PATH;
    static const std::vector<std::pair<const std::string, const std::string>> PREDEFINED_MAPS;

public:
    ColorMap(const std::string& name, const std::string& file_path);

    void bind(unsigned int target = 0);
    void unbind(unsigned int target = 0);

    unsigned int id() const { return m_render_id; }
    const std::string& name() const { return m_name; }

private:
    unsigned int m_render_id;
    std::string m_name;
};

TEKARI_NAMESPACE_END