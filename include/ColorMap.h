#pragma once

#include <nanogui/glutil.h>
#include "stb_image.h"

class ColorMap
{
public:
    ColorMap(const std::string& filePath);

    void bind(unsigned int target = 0);
    void unbind(unsigned int target = 0);

    unsigned int id() const { return m_RenderId; }

private:
    unsigned int m_RenderId;
};