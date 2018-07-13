#pragma once

#include "common.h"
#include "points_stats.h"

TEKARI_NAMESPACE_BEGIN

extern void recompute_data(
    const MatrixXXf& raw_points,
    PointsStats& points_stats,
    VectorXu& path_segments,
    Matrix3Xu& F,
    Matrix2Xf& V2D,
    std::vector<VectorXf>& H, std::vector<VectorXf>& LH,
    std::vector<Matrix3Xf>& N, std::vector<Matrix3Xf>& LN
);


TEKARI_NAMESPACE_END