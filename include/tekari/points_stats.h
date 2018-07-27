#pragma once

#include <tekari/common.h>
#include <tekari/raw_measurement.h>
#include <nanogui/opengl.h>

TEKARI_NAMESPACE_BEGIN

struct PointsStats
{
    size_t      points_count;
    Vector3f    average_point;
    float       average_intensity;
    float       min_intensity;
    float       max_intensity;
    size_t      lowest_point_index;
    size_t      highest_point_index;

    PointsStats();
};

extern void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row& H,
    size_t intensity_index
);

extern void compute_min_max_intensities(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    size_t intensity_index
);

extern void update_points_stats(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row& H,
    size_t intensity_index
);


TEKARI_NAMESPACE_END