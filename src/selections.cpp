#include <tekari/selections.h>
#include <tbb/parallel_for.h>

TEKARI_NAMESPACE_BEGIN

#define MAX_SELECTION_DISTANCE 30.0f

void set_all_points(
    VectorXf& selected_points,
    float value)
{
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)selected_points.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
            selected_points[i] = value;
    });
}

void select_points(
    const Matrix2Xf& V2D,
    const MatrixXXf::Row& H,
    VectorXf& selected_points,
    const Matrix4f & mvp,
    const SelectionBox& selection_box,
    const Vector2i & canvas_size,
    SelectionMode mode)
{
    cout << std::setw(50) << std::left << "Selecting points .. ";
    Timer<> timer;
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)V2D.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            Vector3f point = get_3d_point(V2D, H, i);
            Vector4f proj_point = project_on_screen(point, canvas_size, mvp);

            bool in_selection = selection_box.contains(Vector2i{ proj_point[0], proj_point[1] });

            switch (mode)
            {
            case ADD: in_selection = in_selection || SELECTED(selected_points[i]); break;
            case SUBTRACT: in_selection = !in_selection && SELECTED(selected_points[i]); break;
            default: break;
            }
            selected_points[i] = in_selection ? SELECTED_FLAG : NOT_SELECTED_FLAG;  // arbitrary non-zero value
        }
    });
    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void select_closest_point(
    const Matrix2Xf& V2D,
    const MatrixXXf::Row& H,
    VectorXf& selected_points,
    const Matrix4f& mvp,
    const Vector2i & mouse_pos,
    const Vector2i & canvas_size)
{
    cout << std::setw(50) << std::left << "Selecting closest point .. ";
    Timer<> timer;

    size_t n_threads = V2D.size() / GRAIN_SIZE + ((V2D.size() % GRAIN_SIZE) > 0);
    vector<float> smallest_distances(n_threads, MAX_SELECTION_DISTANCE* MAX_SELECTION_DISTANCE);
    vector<int> closest_point_indices(n_threads, -1);

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)V2D.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
        uint32_t thread_id = range.begin() / GRAIN_SIZE;
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            Vector3f point = get_3d_point(V2D, H, i);
            Vector4f proj_point = project_on_screen(point, canvas_size, mvp);

            float dist_sqr = enoki::squared_norm(Vector2f{ proj_point[0] - mouse_pos[0], proj_point[1] - mouse_pos[1] });

            if (smallest_distances[thread_id] > dist_sqr)
            {
                closest_point_indices[thread_id]   = i;
                smallest_distances[thread_id]     = dist_sqr;
            }

            selected_points[i] = NOT_SELECTED_FLAG;
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
        selected_points[closest_point_index] = SELECTED_FLAG;
    }

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void select_extreme_point(
    const PointsStats& stats,
    VectorXf& selected_points,
    size_t intensity_index,
    bool highest
)
{
    cout << std::setw(50) << std::left << "Selecting extreme point .. ";
    Timer<> timer;

    int point_index = highest ? stats.highest_point_index[intensity_index]: stats.lowest_point_index[intensity_index];
    deselect_all_points(selected_points);
    selected_points[point_index] = SELECTED_FLAG;

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void select_all_points(VectorXf& selected_points)
{
    cout << std::setw(50) << std::left << "Selecting all points .. ";
    Timer<> timer;

    set_all_points(selected_points, SELECTED_FLAG);

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void deselect_all_points(VectorXf& selected_points)
{
    cout << std::setw(50) << std::left << "Deselecting all points .. ";
    Timer<> timer;
    
    set_all_points(selected_points, NOT_SELECTED_FLAG);
    
    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void move_selection_along_path(bool up, VectorXf& selected_points)
{
    cout << std::setw(50) << std::left << "Moving selection along path";
    Timer<> timer;
    
    float extremity;
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
    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void delete_selected_points(
    VectorXf& selected_points,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    PointsStats& selection_info,
    Metadata& metadata
)
{
    cout << std::setw(50) << std::left << "Deleting selection .. ";
    Timer<> timer;

    selection_info = PointsStats();

    Index last_valid = 0;
    for (Index i = 0; i < selected_points.size(); ++i)
    {
        if (!SELECTED(selected_points[i]))
        {
            if (last_valid != i)         // prevent unnecessary copies
            {
                // move undeleted point to last valid position
                V2D[last_valid] = V2D[i];
                raw_measurement[last_valid] = raw_measurement[i];
            }
            ++last_valid;
        }
    }

    // resize vectors
    V2D.resize(last_valid);
    raw_measurement.resize(last_valid, raw_measurement.n_wave_lengths());
    selected_points.resize(last_valid);
    set_all_points(selected_points, NOT_SELECTED_FLAG);

    metadata.set_points_in_file(last_valid);

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

size_t count_selected_points(const VectorXf& selected_points)
{
    cout << std::setw(50) << std::left << "Selecting extreme point .. ";
    Timer<> timer;

    size_t count = 0;
    for (Index i = 0; i < selected_points.size(); ++i) {
        count += SELECTED(selected_points[i]);
    }
    
    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
    return count;
}

TEKARI_NAMESPACE_END