#pragma once

#include "common.h"
#include <string>
#include "Metadata.h"
#include "points_stats.h"

TEKARI_NAMESPACE_BEGIN

extern void load_data_sample(
    const std::string& file_name,
    MatrixXf &raw_points,
    MatrixXf  &V2D,
    VectorXu8 &selected_points,
    Metadata &metadata
);

extern void save_data_sample(
    const std::string& path,
    const MatrixXf &raw_points,
    const Metadata &metadata
);

TEKARI_NAMESPACE_END