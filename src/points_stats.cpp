#include <tekari/points_stats.h>

#include <limits>
#include <iostream>
#include <tekari/selections.h>
#include <tekari/stop_watch.h>

TEKARI_NAMESPACE_BEGIN

PointsStats::PointsStats()
    : m_points_count(0)
{}

void PointsStats::set_size(size_t size)
{
    m_average_point.assign(size, 0);
    m_average_raw_point.assign(size + 2, 0);
    m_lowest_point_index.assign(size, 0);
    m_highest_point_index.assign(size, 0);
    m_min_intensity.assign(size, std::numeric_limits<float>::max());
    m_max_intensity.assign(size, std::numeric_limits<float>::min());
}

void PointsStats::add_point(const RawMeasurement::SamplePoint& sample_point, const Matrix3Xf& transformed_points)
{
    for (Index i = 0; i < sample_point.n_wave_lengths() + 3; ++i)
    {
        m_average_raw_point[i] += sample_point[i];
    }
    for (Index i = 0; i < transformed_points.size(); ++i)
    {
        m_average_point[i] += transformed_points[i];
    }
    ++m_points_count;
}
void PointsStats::add_intensity(size_t index, const RawMeasurement::SamplePoint& sample_point)
{
    for (Index i = 0; i < m_min_intensity.size(); i++)
    {
        if (sample_point[i + 2] < m_min_intensity[i])
        {
            m_lowest_point_index[i] = index;
            m_min_intensity[i] = sample_point[i + 2];
        }
        if (sample_point[i + 2] > m_max_intensity[i])
        {
            m_highest_point_index[i] = index;
            m_max_intensity[i] = sample_point[i + 2];
        }
    }
}

void PointsStats::normalize()
{
    if (m_points_count == 0)
        return;
    for (Index i = 0; i < m_average_raw_point.size(); ++i)
    {
        m_average_raw_point[i] /= m_points_count;
    }
    for (Index i = 0; i < m_average_point.size(); ++i)
    {
        m_average_point[i] /= m_points_count;
    }
}

void PointsStats::normalize_average()
{
    for (Index i = 0; i < m_average_point.size(); i++)
    {
        m_average_point[i][1] = (m_average_point[i][1] - m_min_intensity[i]) / (m_max_intensity[i] - m_min_intensity[i]);
    }
}

void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H)
{
    START_PROFILING("Updating selection statistics");
    selection_stats.m_points_count = 0;
    selection_stats.set_size(raw_measurement.n_wave_lengths() + 1);
    for (Index i = 0; i < selected_points.size(); ++i)
    {
        if (SELECTED(selected_points[i]))
        {
            selection_stats.add_intensity(i, raw_measurement[i]);
            selection_stats.add_point(raw_measurement[i], get_3d_points(V2D, H, i));
        }
    }
    selection_stats.normalize();
    END_PROFILING();
}

void compute_min_max_intensities(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement
)
{
    START_PROFILING("Computing minimum/maximum intensities");

    points_stats.set_size(raw_measurement.n_wave_lengths() + 1);
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        points_stats.add_intensity(i, raw_measurement[i]);
    }
    END_PROFILING();
}

void update_points_stats(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H
)
{
    START_PROFILING("Updating points statistics");

    points_stats.m_points_count = 0;
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        points_stats.add_point(raw_measurement[i], get_3d_points(V2D, H, i));
    }

    points_stats.normalize();
    points_stats.normalize_average();

    END_PROFILING();
}

TEKARI_NAMESPACE_END