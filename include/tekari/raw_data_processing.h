#pragma once

#include <tekari/common.h>
#include <tekari/raw_measurement.h>
#include <tekari/points_stats.h>

TEKARI_NAMESPACE_BEGIN

extern void recompute_data(
    const RawMeasurement& raw_measurement,
    PointsStats& points_stats,
    VectorXu& path_segments,
    Matrix3Xu& F,
    Matrix2Xf& V2D,
    MatrixXXf& H, MatrixXXf& LH,
    Matrix4XXf& N, Matrix4XXf& LN,
    size_t wave_length_index
);


TEKARI_NAMESPACE_END