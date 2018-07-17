#include <tekari/selections.h>

#include <tbb/parallel_for.h>
#include <tekari/stop_watch.h>

TEKARI_NAMESPACE_BEGIN

#define MAX_SELECTION_DISTANCE 30.0f

void select_points(
    const Matrix2Xf& V2D,
    const VectorXf& H,
    VectorXi8& selected_points,
    const Matrix4f & mvp,
    const SelectionBox& selection_box,
    const Vector2i & canvas_size,
    SelectionMode mode)
{
    START_PROFILING("Selecting points");
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)V2D.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            Vector3f point = get3DPoint(V2D, H, i);
            Vector4f proj_point = project_on_screen(point, canvas_size, mvp);

            bool in_selection = selection_box.contains(Vector2i{ proj_point[0], proj_point[1] });

            switch (mode)
            {
            case STANDARD:
                selected_points[i] = in_selection;
                break;
            case ADD:
                selected_points[i] = in_selection || selected_points[i];
                break;
            case SUBTRACT:
                selected_points[i] = !in_selection && selected_points[i];
                break;
            }
        }
    });
    END_PROFILING();
}

void select_closest_point(
    const Matrix2Xf& V2D,
    const VectorXf& H,
    VectorXi8& selected_points,
    const Matrix4f& mvp,
    const Vector2i & mouse_pos,
    const Vector2i & canvas_size)
{
    START_PROFILING("Selecting closest point");
    size_t n_threads = V2D.size() / GRAIN_SIZE + ((V2D.size() % GRAIN_SIZE) > 0);
    vector<float> smallest_distances(n_threads, MAX_SELECTION_DISTANCE* MAX_SELECTION_DISTANCE);
    vector<int> closest_point_indices(n_threads, -1);

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)V2D.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
        uint32_t thread_id = range.begin() / GRAIN_SIZE;
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            Vector3f point = get3DPoint(V2D, H, i);
            Vector4f proj_point = project_on_screen(point, canvas_size, mvp);

            float dist_sqr = enoki::squared_norm(Vector2f{ proj_point[0] - mouse_pos[0], proj_point[1] - mouse_pos[1] });

            if (smallest_distances[thread_id] > dist_sqr)
            {
                closest_point_indices[thread_id]   = i;
                smallest_distances[thread_id]     = dist_sqr;
            }

            selected_points[i] = false;
        }
    });

    float smallest_distance = MAX_SELECTION_DISTANCE* MAX_SELECTION_DISTANCE;
    int closest_point_index = -1;

    for (size_t i = 0; i < smallest_distances.size(); i++)
    {
        if (smallest_distance > smallest_distances[i])
        {
            closest_point_index = closest_point_indices[i];
            smallest_distance = smallest_distances[i];
        }
    }

    if (closest_point_index != -1)
    {
        selected_points[closest_point_index] = true;
    }
    END_PROFILING();
}

void select_extreme_point(
    const PointsStats& points_info,
    const PointsStats& selection_info,
    VectorXi8& selected_points,
    unsigned int wave_length_index,
    bool highest
)
{
    START_PROFILING("Selecting extreme point");
    bool no_selection = selection_info.points_count() == 0;
    int point_index = 
        (highest ? 
            (no_selection ?
                points_info.highest_point_index(wave_length_index)
                : selection_info.highest_point_index(wave_length_index))
            : (no_selection ?
                points_info.lowest_point_index(wave_length_index)
                : selection_info.lowest_point_index(wave_length_index)));

    deselect_all_points(selected_points);
    selected_points[point_index] = 1;

    END_PROFILING();
}

void select_all_points(VectorXi8& selected_points)
{
    START_PROFILING("Selecting all points");
    memset(selected_points.data(), ~0, selected_points.size());
    END_PROFILING();
}

void deselect_all_points(VectorXi8& selected_points)
{
    START_PROFILING("Deselecting all points");
    memset(selected_points.data(), 0, selected_points.size());
    END_PROFILING();
}

void move_selection_along_path(bool up, VectorXi8& selected_points)
{
    START_PROFILING("Moving selection along path");
    uint8_t extremity;
    if (up)
    {
        extremity = selected_points.back();
        memmove(selected_points.data() + 1, selected_points.data(), selected_points.size() - 1);
        selected_points.front() = extremity;
    }
    else
    {
        extremity = selected_points.front();
        memmove(selected_points.data(), selected_points.data() + 1, selected_points.size() - 1);
        selected_points.back() = extremity;
    }
    END_PROFILING();
}

void delete_selected_points(
    VectorXi8& selected_points,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    PointsStats& selection_info,
    Metadata& metadata
)
{
    START_PROFILING("Deleting selection");
    selection_info = PointsStats();

    Index last_valid = 0;
    for (Index i = 0; i < selected_points.size(); ++i)
    {
        if (!selected_points[i])
        {
            if (last_valid != i)         // prevent unnecessary copies
            {
                // move undeleted point to last valid position
                V2D[last_valid] = V2D[i];
                raw_points[last_valid] = std::move(raw_points[i]);
            }
            ++last_valid;
        }
    }

    // resize vectors
    V2D.resize(last_valid);
    raw_points.resize(last_valid);
    selected_points.resize(last_valid);
    memset(selected_points.data(), 0, selected_points.size());

    metadata.set_points_in_file(last_valid);

    END_PROFILING();
}

unsigned int count_selected_points(const VectorXi8& selected_points)
{
    unsigned int count = 0;
    START_PROFILING("Counting selected points");
    for (Index i = 0; i < selected_points.size(); ++i) {
        count += (selected_points[i] != 0);
    }
    END_PROFILING();
    return count;
}

TEKARI_NAMESPACE_END