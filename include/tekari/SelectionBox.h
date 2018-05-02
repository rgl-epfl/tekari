#pragma once

#include <nanogui/glutil.h>

struct SelectionBox
{
    nanogui::Vector2i topLeft;
    nanogui::Vector2i size;

    bool empty() const { return size[0] == 0 || size[1] == 0; }

    bool contains(const nanogui::Vector2i& point) const
    {
        return point[0] >= topLeft[0] && point[0] <= topLeft[0] + size[0] &&
            point[1] >= topLeft[1] && point[1] <= topLeft[1] + size[1];
    }
};