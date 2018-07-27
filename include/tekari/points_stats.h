#pragma once

#include <tekari/common.h>
#include <tekari/raw_measurement.h>
#include <nanogui/opengl.h>

TEKARI_NAMESPACE_BEGIN

class PointsStats
{
public:
    PointsStats();

    void set_size(size_t size);

    size_t    points_count        () const { return m_points_count; }
    Vector3f        average_point       (size_t i) const { return m_average_point[i]; }
    float           average_intensity   (size_t i) const { return m_average_raw_point[i + 2]; }
    float           min_intensity       (size_t i) const { return m_min_intensity[i]; }
    float           max_intensity       (size_t i) const { return m_max_intensity[i]; }
    size_t    highest_point_index (size_t i) const { return m_highest_point_index[i]; }
    size_t    lowest_point_index  (size_t i) const { return m_lowest_point_index[i]; }

    void add_intensity(size_t index, const RawMeasurement::SamplePoint& sample_point);
private:
    void add_point(const RawMeasurement::SamplePoint& sample_point, const Matrix3Xf& transformed_point);

    void normalize_average();
    void normalize();

    size_t m_points_count;
    Matrix3Xf m_average_point;
    VectorXf m_average_raw_point;
    VectorXf m_min_intensity;
    VectorXf m_max_intensity;
    VectorXu m_lowest_point_index;
    VectorXu m_highest_point_index;

    friend void update_selection_stats(
        PointsStats& selection_stats,
        const VectorXf& selected_points,
        const RawMeasurement& raw_measurement,
        const Matrix2Xf& V2D,
        const MatrixXXf& H
    );

    friend void update_points_stats(
        PointsStats& points_stats,
        const RawMeasurement& raw_measurement,
        const Matrix2Xf& V2D,
        const MatrixXXf& H
    );

    friend void compute_min_max_intensities(
        PointsStats& points_stats,
        const RawMeasurement& raw_measurement
    );
};

extern void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H
);

extern void compute_min_max_intensities(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement
);

extern void update_points_stats(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H
);


TEKARI_NAMESPACE_END