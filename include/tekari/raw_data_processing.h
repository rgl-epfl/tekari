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
    VectorXf &H, VectorXf &LH,
    MatrixXf &N, MatrixXf &LN
);


TEKARI_NAMESPACE_END