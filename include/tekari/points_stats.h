#pragma once

#include <tekari/common.h>
#include <tekari/raw_measurement.h>
#include <nanogui/opengl.h>

TEKARI_NAMESPACE_BEGIN

struct PointsStats
{
    size_t intensity_count;
    size_t points_count;
    Matrix3Xf average_point;
    VectorXf average_intensity;
    VectorXf min_intensity;
    VectorXf max_intensity;
    VectorXu lowest_point_index;
    VectorXu highest_point_index;

    PointsStats();
    void reset(size_t intensity_count);
};

extern void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H
);

extern void compute_min_max_intensities(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement
);

extern void update_points_stats(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H
);


TEKARI_NAMESPACE_END