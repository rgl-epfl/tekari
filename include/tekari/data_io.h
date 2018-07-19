#pragma once

#include <tekari/common.h>
#include <tekari/metadata.h>
#include <tekari/points_stats.h>

TEKARI_NAMESPACE_BEGIN

extern void load_data_sample(
    const string& file_name,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    VectorXf& selected_points,
    Metadata& metadata
);

extern void save_data_sample(
    const string& path,
    const MatrixXXf& raw_points,
    const Metadata& metadata
);

TEKARI_NAMESPACE_END