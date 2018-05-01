#pragma once

#include <nanogui/glutil.h>
#include "stb_image.h"
#include "common.h"

TEKARI_NAMESPACE_BEGIN

class ColorMap
{
public:
    static const std::string FOLDER_PATH;
    static const std::vector<std::pair<const std::string, const std::string>> PREDEFINED_MAPS;

public:
    ColorMap(const std::string& name, const std::string& filePath);

    void bind(unsigned int target = 0);
    void unbind(unsigned int target = 0);

    unsigned int id() const { return m_RenderId; }
    const std::string& name() const { return m_Name; }

private:
    unsigned int m_RenderId;
    std::string m_Name;
};

TEKARI_NAMESPACE_END