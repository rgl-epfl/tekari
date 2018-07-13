#pragma once

#include <nanogui/opengl.h>
#include <tbb/parallel_for.h>
#include "points_stats.h"
#include "Metadata.h"

TEKARI_NAMESPACE_BEGIN

enum SelectionMode
{
    STANDARD,
    ADD,
    SUBTRACT
};

struct SelectionBox
{
    Vector2i top_left;
    Vector2i size;

    inline bool empty() const { return size[0] == 0 || size[1] == 0; }

    inline bool contains(const Vector2i& point) const
    {
        return  point[0] >= top_left[0] && point[0] <= top_left[0] + size[0] &&
            point[1] >= top_left[1] && point[1] <= top_left[1] + size[1];
    }
};

extern void select_points(
    const Matrix2Xf& V2D,
    const VectorXf& H,
    VectorXi8& selected_points,
    const Matrix4f & mvp,
    const SelectionBox& selection_box,
    const Vector2i & canvas_size,
    SelectionMode mode
);

extern void select_closest_point(
    const Matrix2Xf& V2D,
    const VectorXf& H,
    VectorXi8& selected_points,
    const Matrix4f& mvp,
    const Vector2i & mouse_pos,
    const Vector2i & canvas_size
);

extern void select_extreme_point(
    const PointsStats& points_info,
    const PointsStats& selection_info,
    VectorXi8& selected_points,
    unsigned int wave_length_index,
    bool highest
);

extern void select_all_points(VectorXi8& selected_points);
extern void deselect_all_points(VectorXi8& selected_points);

extern void move_selection_along_path(bool up, VectorXi8& selected_points);

extern void delete_selected_points(
    VectorXi8& selected_points,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    PointsStats& selection_info,
    Metadata& metadata
);

extern unsigned int count_selected_points(
    const VectorXi8& selected_points
);

TEKARI_NAMESPACE_END