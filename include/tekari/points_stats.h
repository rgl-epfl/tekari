#pragma once

#include <tekari/common.h>
#include <tekari/raw_measurement.h>
#include <nanogui/opengl.h>
#include <limits>

TEKARI_NAMESPACE_BEGIN

class PointsStats
{
public:
    struct Slice
    {
        Vector3f average_point          = Vector3f(0);
        Vector3f average_log_point      = Vector3f(0);
        float average_intensity         = 0.0f;
        float min_intensity             = std::numeric_limits<float>::max();
        float max_intensity             = std::numeric_limits<float>::min();
        uint32_t lowest_point_index     = 0;
        uint32_t highest_point_index    = 0;
    };

    Slice&       operator[](size_t intensity_index)       { return m_slices[intensity_index]; }
    const Slice& operator[](size_t intensity_index) const { return m_slices[intensity_index]; }

    PointsStats();
    void reset(size_t intensity_count);

    size_t intensity_count;
    size_t points_count;
private:
    vector<Slice> m_slices;
};

extern void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H,
    const MatrixXXf& LH,
    size_t intensity_index
);

extern void compute_min_max_intensities(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    size_t intensity_index
);

extern void update_points_stats(
    PointsStats& point_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H,
    const MatrixXXf& LH,
    size_t intensity_index
);


TEKARI_NAMESPACE_END