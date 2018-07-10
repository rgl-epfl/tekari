#include "tekari/RadialGrid.h"

using namespace nanogui;
using namespace std;

TEKARI_NAMESPACE_BEGIN

RadialGrid::RadialGrid()
:    m_color(200, 200, 200, 200)
,    m_visible(true)
,   m_show_degrees(true)
{
    m_shader.init_from_files("grid",
        "../resources/shaders/radial_grid.vert",
        "../resources/shaders/radial_grid.frag");

    vector<Vector3f> vertices(CIRCLE_COUNT * VERTEX_PER_CIRCLE_COUNT +
        LINE_COUNT * VERTEX_PER_LINE_COUNT);

    for (unsigned int i = 0; i < CIRCLE_COUNT; ++i)
    {
        float radius = (float)(i + 1) / CIRCLE_COUNT;
        for (unsigned int j = 0; j < VERTEX_PER_CIRCLE_COUNT; ++j)
        {
            Vector3f point{
                radius * (float)cos(2 * M_PI * j / VERTEX_PER_CIRCLE_COUNT), // x coord
                0,                                              // y coord
                radius * (float)sin(2 * M_PI * j / VERTEX_PER_CIRCLE_COUNT)  // z coord
            };
            vertices[i*VERTEX_PER_CIRCLE_COUNT + j] = point;
        }
        int theta = (i + 1) * 90 / CIRCLE_COUNT;
        if (theta != 0)
        {
            Vector3f &pos = vertices[i*VERTEX_PER_CIRCLE_COUNT];
            m_theta_labels.push_back(make_pair(to_string(theta), pos + Vector3f{0.02f, 0.02f, -0.02f}));
        }
    }

    for (unsigned int i = 0; i < LINE_COUNT; ++i)
    {
        unsigned int index = CIRCLE_COUNT * VERTEX_PER_CIRCLE_COUNT + i * VERTEX_PER_LINE_COUNT;
        double angle = M_PI * i / LINE_COUNT;
        vertices[index] = { (float)cos(angle), 0, (float)sin(angle) };
        vertices[index + 1] = -vertices[index];

        m_phi_labels.push_back(make_pair(to_string(180 * i / LINE_COUNT), vertices[index] + vertices[index].normalized() * 0.04f));
        m_phi_labels.push_back(make_pair(to_string(180 * i / LINE_COUNT + 180), vertices[index+1] + vertices[index + 1].normalized() * 0.04f));
    }

    m_shader.bind();
    m_shader.upload_attrib("in_pos", vertices.size(), 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)vertices.data());
}

RadialGrid::~RadialGrid()
{
    m_shader.free();
}

void RadialGrid::draw_gl(
    const Matrix4f& model,
    const Matrix4f& view,
    const Matrix4f& proj)
{
    if (m_visible)
    {
        Matrix4f mvp = proj * view * model;
        gl_enable(GL_DEPTH_TEST);
        gl_enable(GL_BLEND);
        gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_shader.bind();
        m_shader.set_uniform("model_view_proj", mvp);

        for (size_t j = 0; j < 2; j++)
        {
            gl_depth_func(j % 2 == 0 ? GL_LESS : GL_GREATER);
            m_shader.set_uniform("in_color", j % 2 == 0 ? m_color : m_color * 0.6);
            for (unsigned int i = 0; i < CIRCLE_COUNT; ++i)
            {
                m_shader.draw_array(GL_LINE_LOOP, i*VERTEX_PER_CIRCLE_COUNT, VERTEX_PER_CIRCLE_COUNT);
            }
            m_shader.draw_array(GL_LINES, CIRCLE_COUNT*VERTEX_PER_CIRCLE_COUNT, LINE_COUNT*VERTEX_PER_LINE_COUNT);
        }

        // Restore opengl settings
        gl_depth_func(GL_LESS);
        gl_disable(GL_DEPTH_TEST);
        gl_disable(GL_BLEND);
    }
}

void RadialGrid::draw(  NVGcontext *ctx,
                        const Vector2i &canvas_size,
                        const Matrix4f &model,
                        const Matrix4f &view,
                        const Matrix4f &proj)
{
    if (m_visible && m_show_degrees)
    {
        Matrix4f mvp = proj * view * model;
        nvg_font_size(ctx, 15.0f);
        nvg_font_face(ctx, "sans");
        nvg_text_align(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvg_fill_color(ctx, m_color);
        for (const auto& phi_label : m_phi_labels)
        {
            Vector4f projected_point = project_on_screen(phi_label.second, canvas_size, mvp);
            nvg_text(ctx, projected_point[0], projected_point[1], phi_label.first.c_str(), nullptr);
        }
        for (const auto& theta_label : m_theta_labels)
        {
            Vector4f projected_point = project_on_screen(theta_label.second, canvas_size, mvp);
            nvg_text(ctx, projected_point[0], projected_point[1], theta_label.first.c_str(), nullptr);
        }
    }
}

TEKARI_NAMESPACE_END
