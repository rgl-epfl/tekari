#pragma once

#include <nanogui/opengl.h>
#include <tekari/points_stats.h>
#include <tekari/raw_measurement.h>
#include <tekari/metadata.h>

#define NOT_SELECTED_FLAG 0.0f  // arbitrary zero value
#define SELECTED_FLAG 1.0f  // arbitrary non-zero value
#define SELECTED(point) ((point) > 0.5f)

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
    const MatrixXXf::Row& H,
    VectorXf& selected_points,
    const Matrix4f& mvp,
    const SelectionBox& selection_box,
    const Vector2i& canvas_size,
    SelectionMode mode
);

extern void select_closest_point(
    const Matrix2Xf& V2D,
    const MatrixXXf::Row& H,
    VectorXf& selected_points,
    const Matrix4f& mvp,
    const Vector2i& mouse_pos,
    const Vector2i& canvas_size
);

extern void select_extreme_point(
    const PointsStats& points_stats,
    const PointsStats& selection_stats,
    VectorXf& selected_points,
    size_t intensity_index,
    bool highest
);

extern void select_all_points(VectorXf& selected_points);
extern void deselect_all_points(VectorXf& selected_points);

extern void move_selection_along_path(bool up, VectorXf& selected_points);

extern void delete_selected_points(
    VectorXf& selected_points,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    PointsStats& selection_info,
    Metadata& metadata
);

extern size_t count_selected_points(
    const VectorXf& selected_points
);

TEKARI_NAMESPACE_END