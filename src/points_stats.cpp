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
    average_point.assign(i_count, Vector3f(0));
    average_log_point.assign(i_count, Vector3f(0));
    average_intensity.assign(i_count, 0);
    min_intensity.assign(i_count, std::numeric_limits<float>::max());
    max_intensity.assign(i_count, std::numeric_limits<float>::min());
    lowest_point_index.assign(i_count, 0);
    highest_point_index.assign(i_count, 0);
}

inline void points_stats_add_intensity(PointsStats& points_stats, float intensity, size_t index, size_t intensity_index)
{
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
}

void update_selection_stats(
    PointsStats& selection_stats,
    const VectorXf& selected_points,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row H,
    const MatrixXXf::Row LH,
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
            selection_stats.average_intensity[intensity_index]  += raw_measurement[i][intensity_index + 2];
            selection_stats.average_point[intensity_index]      += get_3d_point(V2D, H, i);
            selection_stats.average_log_point[intensity_index]  += get_3d_point(V2D, LH, i);
        }
    }
    if (selection_stats.points_count != 0)
    {
        float scale = 1.0f / selection_stats.points_count;
        selection_stats.average_intensity[intensity_index] *= scale;
        selection_stats.average_point[intensity_index] *= scale;
        selection_stats.average_log_point[intensity_index] *= scale;
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
        points_stats_add_intensity(points_stats, raw_measurement[i][intensity_index + 2], i, intensity_index);
    END_PROFILING();
}

void update_points_stats(
    PointsStats& points_stats,
    const RawMeasurement& raw_measurement,
    const Matrix2Xf& V2D,
    const MatrixXXf::Row H,
    const MatrixXXf::Row LH,
    size_t intensity_index
)
{
    START_PROFILING("Updating points statistics");
    points_stats.points_count = raw_measurement.n_sample_points();
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        points_stats.average_intensity[intensity_index] += raw_measurement[i][intensity_index + 2];
        points_stats.average_point[intensity_index] += get_3d_point(V2D, H, i);
        points_stats.average_log_point[intensity_index] += get_3d_point(V2D, LH, i);
    }
    if (points_stats.points_count != 0)
    {
        float scale = 1.0f / points_stats.points_count;
        points_stats.average_intensity[intensity_index] *= scale;
        points_stats.average_point[intensity_index] *= scale;
        points_stats.average_log_point[intensity_index] *= scale;

        // // normalize average heights

        // float min_intensity = points_stats.min_intensity[intensity_index];
        // float max_intensity = points_stats.max_intensity[intensity_index];
        // float correction_factor = (min_intensity <= 0.0f ? -min_intensity + CORRECTION_FACTOR : 0.0f);

        // float min_log_intensity = log(min_intensity + correction_factor);
        // float max_log_intensity = log(max_intensity + correction_factor);

        // points_stats.average_point[intensity_index][1] = 
        //     (points_stats.average_point[intensity_index][1] - min_intensity) / (max_intensity - min_intensity);

        // points_stats.average_log_point[intensity_index][1] = 
        //     (log(points_stats.average_log_point[intensity_index][1] + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
    }

    END_PROFILING();
}

TEKARI_NAMESPACE_END