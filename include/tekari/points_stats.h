#pragma once

#include "common.h"

#include <nanogui/opengl.h>
#include <limits>

TEKARI_NAMESPACE_BEGIN

class PointsStats
{
public:
    PointsStats();

    void set_size(unsigned int n_wave_lengths);

    unsigned int    points_count () const { return m_points_count; }
    Vector3f        average_point        (unsigned int wave_length_index) const { return m_average_point[wave_length_index]; }
    float           average_intensity    (unsigned int wave_length_index) const { return m_average_raw_point[wave_length_index + 2]; }
    float           min_intensity        (unsigned int wave_length_index) const { return m_min_intensity[wave_length_index]; }
    float           max_intensity        (unsigned int wave_length_index) const { return m_max_intensity[wave_length_index]; }
    unsigned int    highest_point_index    (unsigned int wave_length_index) const { return m_highest_point_index[wave_length_index]; }
    unsigned int    lowest_point_index    (unsigned int wave_length_index) const { return m_lowest_point_index[wave_length_index]; }

    void add_intensity(unsigned int index, const VectorXf& raw_point);
private:
    void add_point(const VectorXf& raw_point, const Matrix3Xf& transformed_point);

    void normalize_average();
    void normalize();

    unsigned int m_points_count;
    Matrix3Xf m_average_point;
    VectorXf m_average_raw_point;
    VectorXf m_min_intensity;
    VectorXf m_max_intensity;
    VectorXu m_lowest_point_index;
    VectorXu m_highest_point_index;

    friend void update_selection_stats(
        PointsStats& selection_stats,
        const VectorXi8& selected_points,
        const MatrixXXf& raw_points,
        const Matrix2Xf& V2D,
        const std::vector<VectorXf>& H
    );

    friend void update_points_stats(
        PointsStats& points_stats,
        const MatrixXXf& raw_points,
        const Matrix2Xf& V2D,
        const std::vector<VectorXf>& H
    );

    friend void compute_min_max_intensities(
        PointsStats& points_stats,
        const MatrixXXf& raw_points
    );
};

extern void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXi8& selected_points,
    const MatrixXXf& raw_points,
    const Matrix2Xf& V2D,
    const std::vector<VectorXf>& H
);

extern void compute_min_max_intensities(
    PointsStats& points_stats,
    const MatrixXXf& raw_points
);

extern void update_points_stats(
    PointsStats& points_stats,
    const MatrixXXf& raw_points,
    const Matrix2Xf& V2D,
    const std::vector<VectorXf>& H
);


TEKARI_NAMESPACE_END