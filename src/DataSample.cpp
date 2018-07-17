#include <tekari/DataSample.h>

#include <tekari/Arrow.h>

#define MAX_SELECTION_DISTANCE 30.0f

TEKARI_NAMESPACE_BEGIN

DataSample::DataSample()
:   m_wave_length_index(0)
,   m_display_as_log(false)
,   m_display_views{ false, false, true }
,   m_selection_axis{Vector3f{0.0f, 0.0f, 0.0f}}
,   m_dirty(false)
{
    m_draw_functions[PATH] = [this](const Matrix4f& mvp, std::shared_ptr<ColorMap>) {
        if (m_display_views[PATH])
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            m_shaders[PATH].bind();
            m_shaders[PATH].set_uniform("model_view_proj", mvp);
            for (Index i = 0; i < m_path_segments.size() - 1; ++i)
            {
                int offset = m_path_segments[i];
                int count = m_path_segments[i + 1] - m_path_segments[i] - 1;
                m_shaders[PATH].draw_array(GL_LINE_STRIP, offset, count);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    };

    m_draw_functions[POINTS] = [this](const Matrix4f& mvp, std::shared_ptr<ColorMap> color_map) {
        m_shaders[POINTS].bind();
        color_map->bind();
        m_shaders[POINTS].set_uniform("model_view_proj", mvp);
        m_shaders[POINTS].set_uniform("show_all_points", m_display_views[POINTS]);
        m_shaders[POINTS].draw_array(GL_POINTS, 0, m_v2D.size());
    };

    m_draw_functions[INCIDENT_ANGLE] = [this](const Matrix4f& mvp, std::shared_ptr<ColorMap>) {
        if (m_display_views[INCIDENT_ANGLE])
        {
            Vector2f origin2D = transform_raw_point({ m_metadata.incident_theta(), m_metadata.incident_phi() });
            Vector3f origin3D = Vector3f{ origin2D[0], 0.0f, origin2D[1] };
            Arrow::instance().draw_gl(
                origin3D,
                Vector3f(0, 1, 0),
                1.0f,
                mvp,
                Color(1.0f, 0.0f, 1.0f, 1.0f));
        }
    };
}

DataSample::~DataSample()
{
    m_mesh_shader.free();
    for (int i = 0; i != VIEW_COUNT; ++i)
    {
        m_shaders[i].free();
    }
}

void DataSample::draw_gl(
    const Vector3f& view_origin,
    const Matrix4f& model,
    const Matrix4f& view,
    const Matrix4f& proj,
    int flags,
    shared_ptr<ColorMap> color_map)
{
    // Every draw call requires depth testing
    glEnable(GL_DEPTH_TEST);

    // Precompute mvp
    Matrix4f mvp = proj* view* model;

    // Draw the mesh
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0, 2.0);
    m_mesh_shader.bind();
    color_map->bind();
    m_mesh_shader.set_uniform("model_view_proj", mvp);
    m_mesh_shader.set_uniform("model", model);
    m_mesh_shader.set_uniform("view", view_origin);
    m_mesh_shader.set_uniform("use_shadows", (bool)(flags & USES_SHADOWS));
    m_mesh_shader.set_uniform("use_specular", (bool)(flags & USES_SPECULAR));
    m_mesh_shader.draw_indexed(GL_TRIANGLES, 0, m_f.size());
    glDisable(GL_POLYGON_OFFSET_FILL);

    // draw the predicted outgoing angle
    if (flags & DISPLAY_PREDICTED_OUTGOING_ANGLE)
    {
        Vector2f origin2D = transform_raw_point({ m_metadata.incident_theta(), m_metadata.incident_phi() });
        Vector3f origin3D = Vector3f{ origin2D[0], 0.0f, origin2D[1] };
        Arrow::instance().draw_gl(
            -origin3D,
            Vector3f(0, 1, 0),
            1.0f,
            mvp,
            Color(0.0f, 1.0f, 1.0f, 1.0f));
    }
    // call every draw func
    for (const auto& draw_func: m_draw_functions)
        draw_func(mvp, color_map);

    // Don't forget to disable depth testing for later opengl draw calls
    glDisable(GL_DEPTH_TEST);

    // Draw the axis if points are selected
    if (flags & DISPLAY_AXIS && has_selection())
        m_selection_axis.draw_gl(mvp);
}

void DataSample::init_shaders()
{
    const string shader_path = "../resources/shaders/";
    m_mesh_shader.init_from_files("height_map", shader_path + "height_map.vert", shader_path + "height_map.frag");
    m_shaders[PATH].init_from_files("path", shader_path + "path.vert", shader_path + "path.frag");
    m_shaders[POINTS].init_from_files("points", shader_path + "points.vert", shader_path + "points.frag");
}

void DataSample::link_data_to_shaders()
{
    if (m_f.size() == 0)
    {
        throw std::runtime_error("ERROR: cannot link data to shader before loading data.");
    }

    m_mesh_shader.bind();
    m_mesh_shader.set_uniform("color_map", 0);
    m_mesh_shader.upload_attrib("in_pos2d", (float*)m_v2D.data(), 2, m_v2D.size());
    m_mesh_shader.upload_attrib("in_normal", (float*)curr_n().data(), 4, curr_n().size());
    m_mesh_shader.upload_attrib("in_height", curr_h().data(), 1, curr_h().size());
    m_mesh_shader.upload_indices((uint32_t*)m_f.data(), 3, m_f.size());

    m_shaders[PATH].bind();
    m_shaders[PATH].share_attrib(m_mesh_shader, "in_pos2d");
    m_shaders[PATH].share_attrib(m_mesh_shader, "in_height");

    m_shaders[POINTS].bind();
    m_shaders[POINTS].set_uniform("color_map", 0);
    m_shaders[POINTS].share_attrib(m_mesh_shader, "in_pos2d");
    m_shaders[POINTS].share_attrib(m_mesh_shader, "in_height");
    m_shaders[POINTS].upload_attrib("in_selected", m_selected_points.data(), 1, m_selected_points.size());
}

void DataSample::toggle_log_view()
{
    m_display_as_log = !m_display_as_log;

    m_mesh_shader.bind();
    m_mesh_shader.upload_attrib("in_normal", (float*)curr_n().data(), 4, curr_n().size());
    m_mesh_shader.upload_attrib("in_height", curr_h().data(), 1, curr_h().size());

    m_shaders[PATH].bind();
    m_shaders[PATH].share_attrib(m_mesh_shader, "in_height");
    m_shaders[POINTS].bind();
    m_shaders[POINTS].share_attrib(m_mesh_shader, "in_height");

    if (has_selection()) {
        update_selection_stats(m_selection_stats, m_selected_points, m_raw_points, m_v2D, m_display_as_log ? m_lh : m_h);
        m_selection_axis.set_origin(selection_center());
    }
}

void DataSample::update_point_selection()
{
    m_shaders[POINTS].bind();
    m_shaders[POINTS].upload_attrib("in_selected", m_selected_points.data(), 1, m_selected_points.size());

    update_selection_stats(m_selection_stats, m_selected_points, m_raw_points, m_v2D, m_display_as_log ? m_lh : m_h);
    m_selection_axis.set_origin(selection_center());
}

void DataSample::set_wave_length_index(size_t displayed_wave_length)
{
    displayed_wave_length = std::min(displayed_wave_length, m_h.size()-1);
    if (m_wave_length_index == displayed_wave_length)
        return;

    m_wave_length_index = displayed_wave_length;

    m_mesh_shader.bind();
    m_mesh_shader.upload_attrib("in_normal", (float*)curr_n().data(), 4, curr_n().size());
    m_mesh_shader.upload_attrib("in_height", curr_h().data(), 1, curr_h().size());

    m_shaders[PATH].bind();
    m_shaders[PATH].share_attrib(m_mesh_shader, "in_height");
    m_shaders[POINTS].bind();
    m_shaders[POINTS].share_attrib(m_mesh_shader, "in_height");

    m_selection_axis.set_origin(selection_center());
}

TEKARI_NAMESPACE_END