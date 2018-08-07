#pragma once

#include <tekari/common.h>
#include <tekari/raw_measurement.h>
#include <tekari/points_stats.h>

TEKARI_NAMESPACE_BEGIN

extern void triangulate_data(
    Matrix3Xu& F,
    Matrix2Xf& V2D
);

extern void compute_path_segments(
    VectorXu& path_segments,
    const Matrix2Xf& V2D
);

extern void compute_normals(
    const Matrix3Xu& F,
    const Matrix2Xf& V2D,
    const MatrixXXf& H,
    const MatrixXXf& LH,
    Matrix4XXf& N,
    Matrix4XXf& LN,
    size_t intensity_index
);

extern void compute_normalized_heights(
    const RawMeasurement& raw_measurement,
    const PointsStats& point_stats,
    MatrixXXf& H,
    MatrixXXf& LH,
    size_t intensity_index
);

TEKARI_NAMESPACE_END