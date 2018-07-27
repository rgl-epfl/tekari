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

inline void points_stats_add_intensity(PointsStats& points_stats, RawMeasurement::SamplePoint intensities, size_t index)
{
    for (size_t i = 0; i < points_stats.intensity_count; ++i)
    {
        float intensity = intensities[i + 2];
        if (intensity < points_stats.min_intensity[i])
        {
            points_stats.lowest_point_index[i] = index;
            points_stats.min_intensity[i] = intensity;
        }
        if (intensity > points_stats.max_intensity[i])
        {
            points_stats.highest_point_index[i] = index;
            points_stats.max_intensity[i] = intensity;
        }
        points_stats.average_intensity[i] += intensity;
    }
}

void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf& H
)
{
    START_PROFILING("Updating selection statistics");
    selection_stats.reset(raw_measurement.n_wave_lengths() + 1);
    for (Index i = 0; i < selected_points.size(); ++i)
    {
        if (SELECTED(selected_points[i]))
        {
            ++selection_stats.points_count;

            points_stats_add_intensity(selection_stats, raw_measurement[i], i);
            for (size_t j = 0; j < selection_stats.intensity_count; ++j)
            {
                selection_stats.average_point[j] += get_3d_point(V2D, H[j], i);
            }
        }
    }
    if (selection_stats.points_count != 0)
    {
        for (size_t i = 0; i < selection_stats.intensity_count; ++i)
        {
            selection_stats.average_intensity[i] /= selection_stats.points_count;
            selection_stats.average_point[i] /= selection_stats.points_count;
        }
    }
    END_PROFILING();
}

void compute_min_max_intensities(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement
)
{
    START_PROFILING("Computing minimum/maximum intensities");
    points_stats.reset(raw_measurement.n_wave_lengths() + 1);
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        points_stats_add_intensity(points_stats, raw_measurement[i], i);
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
    points_stats.reset(raw_measurement.n_wave_lengths() + 1);
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        ++points_stats.points_count;

        points_stats_add_intensity(points_stats, raw_measurement[i], i);
        for (size_t j = 0; j < points_stats.intensity_count; ++j)
        {
            points_stats.average_point[j] += get_3d_point(V2D, H[j], i);
        }
    }
    if (points_stats.points_count != 0)
    {
        for (size_t i = 0; i < points_stats.intensity_count; ++i)
        {
            points_stats.average_intensity[i] /= points_stats.points_count;
            points_stats.average_point[i] /= points_stats.points_count;

            points_stats.average_point[i][1] = (points_stats.average_point[i][1] - points_stats.min_intensity[i]) /
                                                (points_stats.max_intensity[i] - points_stats.min_intensity[i]);
        }
    }

    END_PROFILING();
}

TEKARI_NAMESPACE_END