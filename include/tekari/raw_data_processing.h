#pragma once

#include "common.h"
#include "points_stats.h"

TEKARI_NAMESPACE_BEGIN

extern void recompute_data(
    const MatrixXf &rawPoints,
    PointsStats &pointsStats,
    VectorXu &pathSegments,
    MatrixXu &F,
    MatrixXf &V2D,
    std::vector<VectorXf> &H, std::vector<VectorXf> &LH,
    std::vector<MatrixXf> &N, std::vector<MatrixXf> &LN
);


TEKARI_NAMESPACE_END