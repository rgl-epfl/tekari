#pragma once

#include "common.h"
#include <string>
#include "Metadata.h"
#include "points_stats.h"

TEKARI_NAMESPACE_BEGIN

extern void load_data_sample(
    const std::string& fileName,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    VectorXu8 &selectedPoints,
    Metadata &metadata
);

extern void save_data_sample(
    const std::string& path,
    const MatrixXf &rawPoints,
    const Metadata &metadata
);

TEKARI_NAMESPACE_END