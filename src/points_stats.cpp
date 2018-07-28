#include <tekari/points_stats.h>

#include <limits>
#include <iostream>
#include <tekari/selections.h>
#include <tekari/stop_watch.h>

TEKARI_NAMESPACE_BEGIN

PointsStats::PointsStats()
: intensity_count(0)
, points_count(0)
{}

void PointsStats::reset(size_t i_count)
{
    intensity_count = i_count;
    points_count = 0;
    average_point.assign(i_count, 0);
    average_intensity.assign(i_count, 0);
    min_intensity.assign(i_count, std::numeric_limits<float>::max());
    max_intensity.assign(i_count, std::numeric_limits<float>::min());
    lowest_point_index.assign(i_count, 0);
    highest_point_index.assign(i_count, 0);
}

inline void points_stats_add_intensity(PointsStats& points_stats, float intensity, size_t index, size_t intensity_index)
{
    // for (size_t i = 0; i < points_stats.intensity_count; ++i)
    // {
        // float intensity = intensities[i + 2];
        if (intensity < points_stats.min_intensity[intensity_index])
        {
            points_stats.lowest_point_index[intensity_index] = index;
            points_stats.min_intensity[intensity_index] = intensity;
        }
        if (intensity > points_stats.max_intensity[intensity_index])
        {
            points_stats.highest_point_index[intensity_index] = index;
            points_stats.max_intensity[intensity_index] = intensity;
        }
        points_stats.average_intensity[intensity_index] += intensity;
    // }
}

void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row H,
    size_t intensity_index
)
{
    START_PROFILING("Updating selection statistics");
    for (Index i = 0; i < selected_points.size(); ++i)
    {
        if (SELECTED(selected_points[i]))
        {
            ++selection_stats.points_count;

            points_stats_add_intensity(selection_stats, raw_measurement[i][intensity_index + 2], i, intensity_index);
            selection_stats.average_point[intensity_index] += get_3d_point(V2D, H, i);
        }
    }
    if (selection_stats.points_count != 0)
    {
        selection_stats.average_intensity[intensity_index] /= selection_stats.points_count;
        selection_stats.average_point[intensity_index] /= selection_stats.points_count;
    }
    END_PROFILING();
}

void compute_min_max_intensities(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    size_t intensity_index
)
{
    START_PROFILING("Computing minimum/maximum intensities");
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        points_stats_add_intensity(points_stats, raw_measurement[i][intensity_index + 2], i, intensity_index);
    }
    END_PROFILING();
}

void update_points_stats(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row H,
    size_t intensity_index
)
{
    START_PROFILING("Updating points statistics");
    points_stats.points_count = raw_measurement.n_sample_points();
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        points_stats_add_intensity(points_stats, raw_measurement[i][intensity_index + 2], i, intensity_index);
        points_stats.average_point[intensity_index] += get_3d_point(V2D, H, i);
    }
    if (points_stats.points_count != 0)
    {
        points_stats.average_intensity[intensity_index] /= points_stats.points_count;
        points_stats.average_point[intensity_index] /= points_stats.points_count;

        points_stats.average_point[intensity_index][1] = 
            (points_stats.average_point[intensity_index][1] - points_stats.min_intensity[intensity_index]) /
            (points_stats.max_intensity[intensity_index] - points_stats.min_intensity[intensity_index]);
    }

    END_PROFILING();
}

TEKARI_NAMESPACE_END