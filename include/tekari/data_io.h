#pragma once

#include <tekari/common.h>
#include <tekari/metadata.h>
#include <tekari/points_stats.h>
#include <tekari/raw_measurement.h>
#include <unordered_map>

TEKARI_NAMESPACE_BEGIN

extern void load_dataset(
    const string& file_name,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    VectorXf& wavelengths,
    Metadata& metadata
);

extern void save_dataset(
    const string& path,
    const RawMeasurement& raw_measurement,
    const Metadata& metadata
);

TEKARI_NAMESPACE_END