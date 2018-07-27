#pragma once

#include <tekari/common.h>
#include <tekari/metadata.h>
#include <tekari/points_stats.h>
#include <tekari/raw_measurement.h>
#include <unordered_map>

TEKARI_NAMESPACE_BEGIN

extern void load_data_sample(
    const string& file_name,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    VectorXf& selected_points,
    Metadata& metadata
);

extern void save_data_sample(
    const string& path,
    const RawMeasurement& raw_measurement,
    const Metadata& metadata
);

TEKARI_NAMESPACE_END