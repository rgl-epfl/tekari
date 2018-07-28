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
    const MatrixXXf::Row H,
    const MatrixXXf::Row LH,
    Matrix4XXf::Row N,
    Matrix4XXf::Row LN
);

extern void compute_normalized_heights(
    const RawMeasurement& raw_measurement,
    PointsStats& points_stats,
    MatrixXXf::Row H,
    MatrixXXf::Row LH,
    size_t intensity_index
);

TEKARI_NAMESPACE_END