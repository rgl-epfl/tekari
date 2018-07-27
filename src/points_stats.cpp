#include <tekari/points_stats.h>

#include <limits>
#include <iostream>
#include <tekari/selections.h>
#include <tekari/stop_watch.h>

TEKARI_NAMESPACE_BEGIN

PointsStats::PointsStats()
: points_count(0)
, average_point(0)
, average_intensity(0)
, min_intensity(std::numeric_limits<float>::max())
, max_intensity(std::numeric_limits<float>::min())
, lowest_point_index(0)
, highest_point_index(0)
{}

inline void points_stats_add_intensity(PointsStats& points_stats, float intensity, size_t index)
{
    if (intensity < points_stats.min_intensity)
    {
        points_stats.lowest_point_index = index;
        points_stats.min_intensity = intensity;
    }
    if (intensity > points_stats.max_intensity)
    {
        points_stats.highest_point_index = index;
        points_stats.max_intensity = intensity;
    }
    points_stats.average_intensity += intensity;
}

void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row& H,
    size_t intensity_index)
{
    START_PROFILING("Updating selection statistics");
    selection_stats = PointsStats();
    for (Index i = 0; i < selected_points.size(); ++i)
    {
        if (SELECTED(selected_points[i]))
        {
            ++selection_stats.points_count;

            float intensity = raw_measurement[i][intensity_index + 2];
            points_stats_add_intensity(selection_stats, intensity, i);
            selection_stats.average_point += get_3d_point(V2D, H, i);
        }
    }
    if (selection_stats.points_count != 0)
    {
        selection_stats.average_intensity /= selection_stats.points_count;
        selection_stats.average_point /= selection_stats.points_count;
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
    points_stats = PointsStats();
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        float intensity = raw_measurement[i][intensity_index + 2];
        points_stats_add_intensity(points_stats, intensity, i);
    }
    END_PROFILING();
}

void update_points_stats(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row& H,
    size_t intensity_index
)
{
    START_PROFILING("Updating points statistics");
    points_stats = PointsStats();
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        ++points_stats.points_count;

        float intensity = raw_measurement[i][intensity_index + 2];
        points_stats_add_intensity(points_stats, intensity, i);
        points_stats.average_point += get_3d_point(V2D, H, i);
    }
    if (points_stats.points_count != 0)
    {
        points_stats.average_intensity /= points_stats.points_count;
        points_stats.average_point /= points_stats.points_count;
    }

    points_stats.average_point[1] = (points_stats.average_point[1] - points_stats.min_intensity) /
                                        (points_stats.max_intensity - points_stats.min_intensity);

    END_PROFILING();
}

TEKARI_NAMESPACE_END