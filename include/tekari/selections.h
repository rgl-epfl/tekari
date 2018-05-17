#pragma once

#include <nanogui/opengl.h>
#include <tbb/parallel_for.h>
#include "points_stats.h"
#include "delaunay.h"

TEKARI_NAMESPACE_BEGIN

enum SelectionMode
{
    STANDARD,
    ADD,
    SUBTRACT
};

struct SelectionBox
{
    nanogui::Vector2i topLeft;
    nanogui::Vector2i size;

    inline bool empty() const { return size[0] == 0 || size[1] == 0; }

    inline bool contains(const nanogui::Vector2i& point) const
    {
        return  point[0] >= topLeft[0] && point[0] <= topLeft[0] + size[0] &&
                point[1] >= topLeft[1] && point[1] <= topLeft[1] + size[1];
    }
};

extern void select_points(
    const std::vector<nanogui::Vector3f> &rawPoints,
    const std::vector<del_point2d_t> &V2D,
    const std::vector<float> &H,
    std::vector<uint8_t> &selectedPoints,
    const nanogui::Matrix4f & mvp,
    const SelectionBox& selectionBox,
    const nanogui::Vector2i & canvasSize,
    SelectionMode mode
);

extern void select_closest_point(
    const std::vector<nanogui::Vector3f> &rawPoints,
    const std::vector<del_point2d_t> &V2D,
    const std::vector<float> &heights,
    std::vector<uint8_t> &selectedPoints,
    const nanogui::Matrix4f& mvp,
    const nanogui::Vector2i & mousePos,
    const nanogui::Vector2i & canvasSize
);

extern void select_highest_point(
    const PointsStats &pointsInfo,
    const PointsStats &selectionInfo,
    std::vector<uint8_t> &selectedPoints
);

extern void deselect_all_points(std::vector<uint8_t> &selectedPoints);

extern void move_selection_along_path(
    bool up,
    std::vector<uint8_t> &selectedPoints
);

extern void delete_selected_points(
    std::vector<uint8_t> &selectedPoints,
    std::vector<nanogui::Vector3f> &rawPoints,
    std::vector<del_point2d_t> &V2D,
    PointsStats &selectionInfo
);

TEKARI_NAMESPACE_END