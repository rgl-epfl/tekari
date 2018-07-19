#include <tekari/points_stats.h>

#include <limits>
#include <iostream>
#include <tekari/selections.h>
#include <tekari/stop_watch.h>

TEKARI_NAMESPACE_BEGIN

PointsStats::PointsStats()
    : m_points_count(0)
{}

void PointsStats::set_size(unsigned int n_wave_lengths)
{
    m_average_point.assign(n_wave_lengths, 0);
    m_average_raw_point.assign(n_wave_lengths + 2, 0);
    m_lowest_point_index.assign(n_wave_lengths, 0);
    m_highest_point_index.assign(n_wave_lengths, 0);
    m_min_intensity.assign(n_wave_lengths, std::numeric_limits<float>::max());
    m_max_intensity.assign(n_wave_lengths, std::numeric_limits<float>::min());
}

void PointsStats::add_point(const VectorXf& raw_point, const Matrix3Xf& transformed_points)
{
    for (Index i = 0; i < raw_point.size(); ++i)
    {
        m_average_raw_point[i] += raw_point[i];
    }
    for (Index i = 0; i < transformed_points.size(); ++i)
    {
        m_average_point[i] += transformed_points[i];
    }
    ++m_points_count;
}
void PointsStats::add_intensity(unsigned int index, const VectorXf& raw_point)
{
    for (Index i = 0; i < m_min_intensity.size(); i++)
    {
        if (raw_point[i + 2] < m_min_intensity[i])
        {
            m_lowest_point_index[i] = index;
            m_min_intensity[i] = raw_point[i + 2];
        }
        if (raw_point[i + 2] > m_max_intensity[i])
        {
            m_highest_point_index[i] = index;
            m_max_intensity[i] = raw_point[i + 2];
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
    const MatrixXXf& raw_points,
    const Matrix2Xf& V2D,
    const vector<VectorXf>& H)
{
    START_PROFILING("Updating selection statistics");
    selection_stats.m_points_count = 0;
    selection_stats.set_size(raw_points[0].size() - 2);
    for (Index i = 0; i < selected_points.size(); ++i)
    {
        if (SELECTED(selected_points[i]))
        {
            selection_stats.add_intensity(i, raw_points[i]);
            selection_stats.add_point(raw_points[i], get3DPoints(V2D, H, i));
        }
    }
    selection_stats.normalize();
    END_PROFILING();
}

void compute_min_max_intensities(
    PointsStats& points_stats,
    const MatrixXXf& raw_points
)
{
    START_PROFILING("Computing minimum/maximum intensities");

    points_stats.set_size(raw_points[0].size() - 2);
    for (Index i = 0; i < raw_points.size(); ++i)
    {
        points_stats.add_intensity(i, raw_points[i]);
    }
    END_PROFILING();
}

void update_points_stats(
    PointsStats& points_stats,
    const MatrixXXf& raw_points,
    const Matrix2Xf& V2D,
    const vector<VectorXf>& H
)
{
    START_PROFILING("Updating points statistics");

    points_stats.m_points_count = 0;
    for (Index i = 0; i < raw_points.size(); ++i)
    {
        points_stats.add_point(raw_points[i], get3DPoints(V2D, H, i));
    }

    points_stats.normalize();
    points_stats.normalize_average();

    END_PROFILING();
}

TEKARI_NAMESPACE_END