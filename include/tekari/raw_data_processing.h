#pragma once

#include "common.h"
#include "points_stats.h"

TEKARI_NAMESPACE_BEGIN

extern void recompute_data(
    const std::vector<nanogui::Vector3f> &rawPoints,
    PointsStats &pointsStats,
    tri_delaunay2d_t **triangulation,
    std::vector<unsigned int> &pathSegments,
    std::vector<del_point2d_t> &V2D,
    std::vector<float> &H,
    std::vector<float> &LH,
    std::vector<nanogui::Vector3f> &N,
    std::vector<nanogui::Vector3f> &LN
);


TEKARI_NAMESPACE_END