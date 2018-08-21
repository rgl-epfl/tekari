#include <tekari/data_sample.h>

#include <tekari/raw_data_processing.h>
#include <tekari/arrow.h>
#include <tekari_resources.h>

#define MAX_SELECT_DISTANCE 30.0f

TEKARI_NAMESPACE_BEGIN

DataSample::DataSample()
:   m_intensity_index(0)
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
    float point_size,
    shared_ptr<ColorMap> color_map)
{
    Vector3f origin3D = enoki::concat(hemisphere_to_disk(m_metadata.incident_angle()), 0.0f);

    // draw the predicted outgoing angle
    if (flags & DISPLAY_PREDICTED_OUTGOING_ANGLE)
    {
        glEnable(GL_DEPTH_TEST);
        Arrow::instance().draw_gl(
            -origin3D,
            Vector3f(0, 0, 1),
            1.2f,
            mvp,
            Color(0.0f, 1.0f, 1.0f, 1.0f));
        glDisable(GL_DEPTH_TEST);
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
            m_shaders[MESH].set_uniform("use_integrated_colors", (m_intensity_index == 0 && !m_wavelengths.empty()) || bool(flags & USE_INTEGRATED_COLORS));
            m_shaders[MESH].draw_indexed(GL_TRIANGLES, 0, m_f.n_rows());
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
            for (size_t i = 0; i < m_path_segments.size() - 1; ++i)
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
#if defined(EMSCRIPTEN)
        m_shaders[POINTS].set_uniform("point_size", point_size);
#else
        glPointSize(point_size);
#endif
        m_shaders[POINTS].set_uniform("model_view_proj", mvp);
        m_shaders[POINTS].set_uniform("show_all_points", m_display_views[POINTS]);
        m_shaders[POINTS].draw_array(GL_POINTS, 0, m_v2d.size());
    };

    draw_functors[INCIDENT_ANGLE] = [&]() {
        if (m_display_views[INCIDENT_ANGLE])
        {
            glEnable(GL_DEPTH_TEST);
            Arrow::instance().draw_gl(
                origin3D,
                Vector3f(0, 0, 1),
                1.2f,
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

void DataSample::init()
{
    m_shaders[MESH].init("height_map", VERTEX_SHADER_STR(height_map), FRAGMENT_SHADER_STR(height_map));
    m_shaders[PATH].init("path", VERTEX_SHADER_STR(path), FRAGMENT_SHADER_STR(path));
    m_shaders[POINTS].init("points", VERTEX_SHADER_STR(points), FRAGMENT_SHADER_STR(points));
}

void DataSample::link_data_to_shaders()
{
    if (m_f.n_rows() == 0)
        throw std::runtime_error("ERROR: cannot link data to shader before loading data.");

    m_shaders[MESH].bind();
    m_shaders[MESH].set_uniform("color_map", 0);
    m_shaders[MESH].upload_attrib("in_pos2d", (float*)m_v2d.data(), 2, m_v2d.size());
    m_shaders[MESH].upload_attrib("in_color", (float*)m_colors.data(), 3, m_colors.n_rows());
    m_shaders[MESH].upload_indices((int*)m_f.data(), 3, m_f.n_rows());

    m_shaders[PATH].bind();
    m_shaders[PATH].share_attrib(m_shaders[MESH], "in_pos2d");

    m_shaders[POINTS].bind();
    m_shaders[POINTS].share_attrib(m_shaders[MESH], "in_pos2d");
    m_shaders[POINTS].set_uniform("color_map", 0);
    m_shaders[POINTS].upload_attrib("in_selected", m_selected_points.data(), 1, m_selected_points.size());
}

void DataSample::toggle_log_view()
{
    m_display_as_log = !m_display_as_log;
    update_shaders_data();
}

void DataSample::update_point_selection()
{
    m_shaders[POINTS].bind();
    m_shaders[POINTS].upload_attrib("in_selected", m_selected_points.data(), 1, m_selected_points.size());

    m_selection_stats.reset(m_raw_measurement.n_wavelengths() + 1);
    update_selection_stats( m_selection_stats, m_selected_points, m_raw_measurement, m_v2d, m_h, m_intensity_index);
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
    m_selection_axis.set_origin(selection_center());
}

void DataSample::recompute_data()
{
    triangulate_data(m_f, m_v2d);
    compute_path_segments(m_path_segments, m_v2d);

    size_t n_intensities = m_raw_measurement.n_wavelengths() + 1;     // account for luminance
    size_t n_sample_points = m_raw_measurement.n_sample_points();
    m_h[0].resize(n_intensities, n_sample_points);
    m_h[1].resize(n_intensities, n_sample_points);
    m_n[0].resize(n_intensities, n_sample_points);
    m_n[1].resize(n_intensities, n_sample_points);
    m_selected_points.assign(n_sample_points, NOT_SELECTED_FLAG);
    m_cache_mask.resize(n_intensities);
    m_points_stats.reset(n_intensities);
    m_selection_stats.reset(n_intensities);
}

TEKARI_NAMESPACE_END
