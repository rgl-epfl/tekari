#include <tekari/data_sample.h>

#include <tekari/raw_data_processing.h>
#include <tekari/arrow.h>
#include <tekari_resources.h>

#define MAX_SELECTION_DISTANCE 30.0f

TEKARI_NAMESPACE_BEGIN

DataSample::DataSample()
:   m_intensity_index(-1)
,   m_display_as_log(false)
,   m_display_views{ true, false, false, true }
,   m_selection_axis{Vector3f{0.0f, 0.0f, 0.0f}}
,   m_dirty(false)
{}

DataSample::~DataSample()
{
    for (int i = 0; i != VIEW_COUNT; ++i)
        m_shaders[i].free();
}

void DataSample::draw_gl(
    const Matrix4f& model,
    const Matrix4f& mvp,
    int flags,
    shared_ptr<ColorMap> color_map)
{
    // draw the predicted outgoing angle
    if (flags & DISPLAY_PREDICTED_OUTGOING_ANGLE)
    {
        Vector2f origin2D = hemisphere_to_disk(m_metadata.incident_angle());
        Vector3f origin3D = Vector3f{ origin2D[0], origin2D[1], 0.0f };
        Arrow::instance().draw_gl(
            -origin3D,
            Vector3f(0, 0, 1),
            1.0f,
            mvp,
            Color(0.0f, 1.0f, 1.0f, 1.0f));
    }

    // all the draw functions
    function<void(void)> draw_functors[VIEW_COUNT];

    draw_functors[MESH] = [&]() {
        if (m_display_views[MESH])
        {
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_POLYGON_OFFSET_FILL);
#if !defined(EMSCRIPTEN)
            if (flags & USE_WIREFRAME)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
            glPolygonOffset(2.0, 2.0);
            m_shaders[MESH].bind();
            color_map->bind();
            m_shaders[MESH].set_uniform("model_view_proj", mvp);
            m_shaders[MESH].set_uniform("model", model);
            m_shaders[MESH].set_uniform("inverse_transpose_model", enoki::inverse_transpose(model));
            m_shaders[MESH].set_uniform("use_shadows", (bool)(flags & USE_SHADOWS));
            m_shaders[MESH].set_uniform("use_specular", (bool)(flags & USE_SPECULAR));
            m_shaders[MESH].draw_indexed(GL_TRIANGLES, 0, m_f.size());
#if !defined(EMSCRIPTEN)
            if (flags & USE_WIREFRAME)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
            glDisable(GL_POLYGON_OFFSET_FILL);
            glDisable(GL_DEPTH_TEST);
        }
    };

    draw_functors[PATH] = [&]() {
        if (m_display_views[PATH])
        {
            glEnable(GL_DEPTH_TEST);
            m_shaders[PATH].bind();
            m_shaders[PATH].set_uniform("model_view_proj", mvp);
            for (Index i = 0; i < m_path_segments.size() - 1; ++i)
            {
                int offset = m_path_segments[i];
                int count = m_path_segments[i + 1] - m_path_segments[i] - 1;
                m_shaders[PATH].draw_array(GL_LINE_STRIP, offset, count);
            }
            glDisable(GL_DEPTH_TEST);
        }
    };

    draw_functors[POINTS] = [&]() {
        m_shaders[POINTS].bind();
        color_map->bind();
        m_shaders[POINTS].set_uniform("model_view_proj", mvp);
        m_shaders[POINTS].set_uniform("show_all_points", m_display_views[POINTS]);
        m_shaders[POINTS].draw_array(GL_POINTS, 0, m_v2d.size());
    };

    draw_functors[INCIDENT_ANGLE] = [&]() {
        if (m_display_views[INCIDENT_ANGLE])
        {
            glEnable(GL_DEPTH_TEST);
            Vector2f origin2D = hemisphere_to_disk(m_metadata.incident_angle());
            Vector3f origin3D = Vector3f{ origin2D[0], origin2D[1], 0.0f };
            Arrow::instance().draw_gl(
                origin3D,
                Vector3f(0, 0, 1),
                1.0f,
                mvp,
                Color(1.0f, 0.0f, 1.0f, 1.0f));
            glDisable(GL_DEPTH_TEST);
        }
    };

    // call every draw func
    for (const auto& draw_func: draw_functors)
        draw_func();

    // Draw the axis if points are selected
    if (flags & DISPLAY_AXIS && has_selection())
        m_selection_axis.draw_gl(mvp);
}

void DataSample::init_shaders()
{
    m_shaders[MESH].init("height_map", VERTEX_SHADER_STR(height_map), FRAGMENT_SHADER_STR(height_map));
    m_shaders[PATH].init("path", VERTEX_SHADER_STR(path), FRAGMENT_SHADER_STR(path));
    m_shaders[POINTS].init("points", VERTEX_SHADER_STR(points), FRAGMENT_SHADER_STR(points));
}

void DataSample::link_data_to_shaders()
{
    if (m_f.size() == 0)
        throw std::runtime_error("ERROR: cannot link data to shader before loading data.");

    m_shaders[MESH].bind();
    m_shaders[MESH].set_uniform("color_map", 0);
    m_shaders[MESH].upload_attrib("in_pos2d", (float*)m_v2d.data(), 2, m_v2d.size());
    // m_shaders[MESH].upload_attrib("in_normal", (float*)curr_n().data(), 4, curr_n().n_cols());
    m_shaders[MESH].upload_indices((uint32_t*)m_f.data(), 3, m_f.size());

    m_shaders[PATH].bind();
    m_shaders[PATH].share_attrib(m_shaders[MESH], "in_pos2d");

    m_shaders[POINTS].bind();
    m_shaders[POINTS].set_uniform("color_map", 0);
    m_shaders[POINTS].share_attrib(m_shaders[MESH], "in_pos2d");
    m_shaders[POINTS].upload_attrib("in_selected", m_selected_points.data(), 1, m_selected_points.size());
}

void DataSample::toggle_log_view()
{
    m_display_as_log = !m_display_as_log;
    update_shaders_data();
    m_selection_axis.set_origin(selection_center());
}

void DataSample::update_point_selection()
{
    m_shaders[POINTS].bind();
    m_shaders[POINTS].upload_attrib("in_selected", m_selected_points.data(), 1, m_selected_points.size());

    m_selection_stats.reset(m_raw_measurement.n_wave_lengths() + 1);
    update_selection_stats( m_selection_stats, m_selected_points, m_raw_measurement, m_v2d, m_h, m_lh, m_intensity_index);
    m_selection_axis.set_origin(selection_center());
}

void DataSample::update_shaders_data()
{
    m_shaders[MESH].bind();
    m_shaders[MESH].upload_attrib("in_height", curr_h().data(), 1, curr_h().n_cols());
    m_shaders[MESH].upload_attrib("in_normal", (float*)curr_n().data(), 4, curr_n().n_cols());

    m_shaders[PATH].bind();
    m_shaders[PATH].share_attrib(m_shaders[MESH], "in_height");
    m_shaders[POINTS].bind();
    m_shaders[POINTS].share_attrib(m_shaders[MESH], "in_height");
}

void DataSample::select_points(const Matrix4f& mvp, const SelectionBox& selection_box, const Vector2i& canvas_size, SelectionMode mode)
{
    tekari::select_points(m_v2d, curr_h(), m_selected_points, mvp, selection_box, canvas_size, mode);
    update_point_selection();
}
void DataSample::select_closest_point(const Matrix4f& mvp, const Vector2i& mouse_pos, const Vector2i& canvas_size)
{
    tekari::select_closest_point(m_v2d, curr_h(), m_selected_points, mvp, mouse_pos, canvas_size);
    update_point_selection();
}
void DataSample::select_extreme_point(bool highest)
{
    if (m_selection_stats.points_count <= 1)
        tekari::select_extreme_point( m_points_stats, m_selected_points, m_intensity_index, highest);
    else
        tekari::select_extreme_point( m_selection_stats, m_selected_points, m_intensity_index, highest);

    update_point_selection();
}
void DataSample::select_all_points()
{
    tekari::select_all_points(m_selected_points);
    update_point_selection();

}
void DataSample::deselect_all_points()
{
    tekari::deselect_all_points(m_selected_points);
    update_point_selection();
}
void DataSample::move_selection_along_path(bool up)
{
    tekari::move_selection_along_path(up, m_selected_points);
    update_point_selection();
}
void DataSample::delete_selected_points()
{

    tekari::delete_selected_points(m_selected_points, m_raw_measurement, m_v2d, m_selection_stats, m_metadata);
    recompute_data();
    link_data_to_shaders();

    // clear mask
    std::fill(m_cache_mask.begin(), m_cache_mask.end(), false);
    set_intensity_index(m_intensity_index);
}
size_t DataSample::count_selected_points() const
{
    return tekari::count_selected_points(m_selected_points);
}
void DataSample::recompute_data()
{
    triangulate_data(m_f, m_v2d);
    compute_path_segments(m_path_segments, m_v2d);

    Index n_intensities = m_raw_measurement.n_wave_lengths() + 1;     // account for luminance
    Index n_sample_points = m_raw_measurement.n_sample_points();
    m_h.resize (n_intensities, n_sample_points);
    m_lh.resize(n_intensities, n_sample_points);
    m_n.resize (n_intensities, n_sample_points);
    m_ln.resize(n_intensities, n_sample_points);
    m_cache_mask.resize(n_intensities);
    m_points_stats.reset(n_intensities);
    m_selection_stats.reset(n_intensities);
}

void DataSample::save(const string& path)
{
    save_data_sample(path, m_raw_measurement, m_metadata);
}

TEKARI_NAMESPACE_END