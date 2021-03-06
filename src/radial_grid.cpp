#include <tekari/radial_grid.h>
#include <tekari_resources.h>

TEKARI_NAMESPACE_BEGIN

RadialGrid::RadialGrid()
:    m_color(200, 200, 200, 200)
,    m_visible(true)
,   m_show_degrees(true)
{
    if (!m_shader.init("grid", VERTEX_SHADER_STR(radial_grid), FRAGMENT_SHADER_STR(radial_grid)))
    {
        cerr << "Unable to load radial grid shader" << endl;
        exit(-1);
    }

    vector<Vector2f> vertices(  CIRCLE_COUNT * VERTEX_PER_CIRCLE_COUNT +
                                LINE_COUNT * VERTEX_PER_LINE_COUNT);

    for (size_t i = 0; i < CIRCLE_COUNT; ++i)
    {
        float radius = (float)(i + 1) / CIRCLE_COUNT;
        for (size_t j = 0; j < VERTEX_PER_CIRCLE_COUNT; ++j)
        {
            float angle = static_cast<float>(2 * M_PI * j / VERTEX_PER_CIRCLE_COUNT);
            Vector2f point{
                radius * cos(angle),
                radius * sin(angle),
            };
            vertices[i*VERTEX_PER_CIRCLE_COUNT + j] = point;
        }
        int theta = (i + 1) * 90 / CIRCLE_COUNT;
        if (theta != 0)
        {
            Vector3f pos = enoki::concat(vertices[i * VERTEX_PER_CIRCLE_COUNT], 0.0f);
            m_theta_labels.push_back(make_pair(to_string(theta), pos));
        }
    }

    for (size_t i = 0; i < LINE_COUNT; ++i)
    {
        size_t index = CIRCLE_COUNT* VERTEX_PER_CIRCLE_COUNT + i* VERTEX_PER_LINE_COUNT;
        float angle = static_cast<float>(M_PI * i / LINE_COUNT);
        float cosa = cos(angle);
        float sina = sin(angle);
        vertices[index]     = Vector2f{ cosa, sina };
        vertices[index+1]   = Vector2f{ -cosa, -sina };

        Vector3f pos0 = Vector3f(cosa, sina, 0.0f);
        Vector3f pos1 = Vector3f(-cosa, -sina, 0.0f);
        int deg = static_cast<int>(180 * i / LINE_COUNT);
        m_phi_labels.push_back(make_pair(to_string(deg), pos0 + enoki::normalize(pos0) * 0.04f));
        m_phi_labels.push_back(make_pair(to_string(deg - 180), pos1 + enoki::normalize(pos1) * 0.04f));
    }

    m_shader.bind();
    m_shader.upload_attrib("in_pos", (float*)vertices.data(), 2, vertices.size());
}

RadialGrid::~RadialGrid()
{
    m_shader.free();
}

void RadialGrid::draw_gl(
    const Matrix4f& mvp)
{
    if (m_visible)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_shader.bind();
        m_shader.set_uniform("model_view_proj", mvp);

        for (size_t j = 0; j < 2; j++)
        {
            glDepthFunc(j % 2 == 0 ? GL_LESS : GL_GREATER);
            m_shader.set_uniform("in_color", j % 2 == 0 ? m_color : m_color * 0.6f);
            for (size_t i = 0; i < CIRCLE_COUNT; ++i)
            {
                m_shader.draw_array(GL_LINE_LOOP, i*VERTEX_PER_CIRCLE_COUNT, VERTEX_PER_CIRCLE_COUNT);
            }
            m_shader.draw_array(GL_LINES, CIRCLE_COUNT*VERTEX_PER_CIRCLE_COUNT, LINE_COUNT*VERTEX_PER_LINE_COUNT);
        }

        // Restore opengl settings
        glDepthFunc(GL_LESS);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
}

void RadialGrid::draw(  NVGcontext* ctx,
                        const Vector2i& canvas_size,
                        const Matrix4f& mvp)
{
    if (m_visible && m_show_degrees)
    {
        nvgFontSize(ctx, 15.0f);
        nvgFontFace(ctx, "sans");
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(ctx, m_color);
        for (const auto& phi_label : m_phi_labels)
        {
            Vector4f projected_point = project_on_screen(phi_label.second, canvas_size, mvp);
            nvgText(ctx, projected_point[0], projected_point[1], phi_label.first.c_str(), nullptr);
        }
        for (const auto& theta_label : m_theta_labels)
        {
            Vector4f projected_point = project_on_screen(theta_label.second, canvas_size, mvp);
            nvgText(ctx, projected_point[0] + 10, projected_point[1] - 10, theta_label.first.c_str(), nullptr);
        }
    }
}

TEKARI_NAMESPACE_END
