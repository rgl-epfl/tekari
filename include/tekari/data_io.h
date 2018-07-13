#pragma once

#include "common.h"
#include <string>
#include "Metadata.h"
#include "points_stats.h"

TEKARI_NAMESPACE_BEGIN

extern void load_data_sample(
    const std::string& file_name,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    VectorXi8& selected_points,
    Metadata& metadata
);

extern void save_data_sample(
    const std::string& path,
    const MatrixXXf& raw_points,
    const Metadata& metadata
);

TEKARI_NAMESPACE_END