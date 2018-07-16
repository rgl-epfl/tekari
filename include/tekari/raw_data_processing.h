#pragma once

#include <tekari/common.h>
#include <tekari/points_stats.h>

TEKARI_NAMESPACE_BEGIN

extern void recompute_data(
    const MatrixXXf& raw_points,
    PointsStats& points_stats,
    VectorXu& path_segments,
    Matrix3Xu& F,
    Matrix2Xf& V2D,
    vector<VectorXf>& H, vector<VectorXf>& LH,
    vector<Matrix4Xf>& N, vector<Matrix4Xf>& LN
);


TEKARI_NAMESPACE_END