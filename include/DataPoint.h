#pragma once

#include "common.h"
#include <cmath>
#include "delaunay.h"

struct RawDataPoint
{
    float theta;
    float phi;
    float intensity;

    del_point2d_t transform() const
    {
        return del_point2d_t{ (float)(theta * cos(phi * M_PI / 180.0f) / 90.0f), (float)(theta * sin(phi * M_PI / 180.0f) / 90.0f) };
    }
};
