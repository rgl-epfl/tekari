#include "tekari/points_stats.h"

#include <iostream>
#include "tekari/selections.h"
#include "tekari/stop_watch.h"

TEKARI_NAMESPACE_BEGIN

using namespace nanogui;
using namespace std;

PointsStats::PointsStats()
    : m_points_count(0)
{}

void PointsStats::set_size(unsigned int n_wave_lengths)
{
    m_average_point.resize(3, n_wave_lengths);
    m_average_raw_point.resize(n_wave_lengths + 2);
    m_min_intensity.resize(n_wave_lengths);
    m_max_intensity.resize(n_wave_lengths);
    m_lowest_point_index.resize(n_wave_lengths);
    m_highest_point_index.resize(n_wave_lengths);

    m_average_point.set_zero();
    m_average_raw_point.set_zero();
    m_min_intensity.set_constant(numeric_limits<float>::max());
    m_max_intensity.set_constant(numeric_limits<float>::min());
    m_lowest_point_index.set_zero();
    m_highest_point_index.set_zero();
}

void PointsStats::add_point(const VectorXf& raw_point, const MatrixXf& transformed_point)
{
    m_average_point += transformed_point;
    m_average_raw_point += raw_point;
    ++m_points_count;
}
void PointsStats::add_intensity(unsigned int index, const VectorXf &raw_point)
{
    for (Eigen::Index i = 0; i < m_min_intensity.size(); i++)
    {
        if (raw_point(i + 2) < m_min_intensity(i))
        {
            m_lowest_point_index(i) = index;
            m_min_intensity(i) = raw_point(i + 2);
        }
        if (raw_point(i + 2) > m_max_intensity(i))
        {
            m_highest_point_index(i) = index;
            m_max_intensity(i) = raw_point(i + 2);
        }
    }
}

void PointsStats::normalize()
{
    if (m_points_count == 0)
        return;
    m_average_point /= m_points_count;
    m_average_raw_point /= m_points_count;
}

void PointsStats::normalize_average()
{
    for (Eigen::Index i = 0; i < m_average_point.cols(); i++)
    {
        m_average_point(1, i) = (m_average_point(1, i) - m_min_intensity(i)) / (m_max_intensity(i) - m_min_intensity(i));
    }
}

void update_selection_stats(
    PointsStats &selection_stats,
    const VectorXu8 &selected_points,
    const MatrixXf &raw_points,
    const MatrixXf &V2D,
    const vector<VectorXf> &H)
{
    START_PROFILING("Updating selection statistics");
    selection_stats.m_points_count = 0;
    selection_stats.set_size(raw_points.rows() - 2);
    for (Eigen::Index i = 0; i < selected_points.size(); ++i)
    {
        if (selected_points(i))
        {
            selection_stats.add_intensity(i, raw_points.col(i));
            selection_stats.add_point(raw_points.col(i), get3DPoints(V2D, H, i));
        }
    }
    selection_stats.normalize();
    END_PROFILING();
}

void compute_min_max_intensities(
    PointsStats &points_stats,
    const MatrixXf &raw_points
)
{
    START_PROFILING("Computing minimum/maximum intensities");

    points_stats.set_size(raw_points.rows() - 2);
    for (Eigen::Index i = 0; i < raw_points.cols(); ++i)
    {
        points_stats.add_intensity(i, raw_points.col(i));
    }
    END_PROFILING();
}

void update_points_stats(
    PointsStats &points_stats,
    const MatrixXf &raw_points,
    const MatrixXf &V2D,
    const vector<VectorXf> &H
)
{
    START_PROFILING("Updating points statistics");

    points_stats.m_points_count = 0;
    for (Eigen::Index i = 0; i < raw_points.cols(); ++i)
    {
        points_stats.add_point(raw_points.col(i), get3DPoints(V2D, H, i));
    }

    points_stats.normalize();
    points_stats.normalize_average();

    END_PROFILING();
}

TEKARI_NAMESPACE_END