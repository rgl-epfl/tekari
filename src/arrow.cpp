#include <tekari/arrow.h>

#include <enoki/transform.h>
#include <enoki/quaternion.h>
#include <tekari_resources.h>

#define CIRCLE_VERTEX_COUNT 20u                // The number of vertices per circle
#define CONE_CYLINDER_HEIGHT_RATIO 0.9f    // The ratio between the cone and cylinder's heights
#define CYLINDER_RADIUS 0.005f                // The radius of the cylinder
#define CONE_CYLINDER_RADIUS_RATIO 2.0f     // The ratio between the cone and cylinder's radius

TEKARI_NAMESPACE_BEGIN

Arrow& Arrow::instance()
{
    static Arrow instance;
    return instance;
}

Arrow::~Arrow()
{
    m_cone_shader.free();
    m_cylinder_shader.free();
}

void Arrow::draw_gl(const Vector3f& origin,
                    const Vector3f& direction,
                    float s,
                    const Matrix4f& vp,
                    const Color& color)
{
    using namespace enoki;
    glEnable(GL_DEPTH_TEST);

    Matrix4f mvp =
        vp * enoki::look_at<Matrix4f>(origin, origin+direction, normalize(Vector3f(1, 2, 3))) *
        enoki::scale<Matrix4f>(Vector3f(1.f, 1.f, s));

    m_cone_shader.bind();
    m_cone_shader.set_uniform("model_view_proj", mvp);
    m_cone_shader.set_uniform("color", color);
    m_cone_shader.draw_array(GL_TRIANGLE_FAN, 0, CIRCLE_VERTEX_COUNT+2);

    m_cylinder_shader.bind();
    m_cylinder_shader.set_uniform("model_view_proj", mvp);
    m_cylinder_shader.set_uniform("color", color);
    m_cylinder_shader.draw_array(GL_TRIANGLE_STRIP, 0, (CIRCLE_VERTEX_COUNT+1)*2);
    
    glDisable(GL_DEPTH_TEST);
}

void Arrow::load_shaders()
{
    if (!m_cone_shader.init("cone", VERTEX_SHADER_STR(arrow), FRAGMENT_SHADER_STR(arrow)) ||
        !m_cylinder_shader.init("cylinder", VERTEX_SHADER_STR(arrow), FRAGMENT_SHADER_STR(arrow)))
    {
        Log(Error, "%s", "Unable to load shaders for arrow");
        exit(-1);
    }

    vector<Vector4f> cone_vertices(CIRCLE_VERTEX_COUNT + 2);
    vector<Vector4f> cylinder_vertices(2 * (CIRCLE_VERTEX_COUNT + 1));

    cone_vertices[0] = Vector4f(0.0f, 0.0f, 1.0f, 1.0f);
    for (size_t i = 0; i <= CIRCLE_VERTEX_COUNT; ++i)
    {
        float angle = M_PI * 2.0f * i / CIRCLE_VERTEX_COUNT;
        auto sc = enoki::sincos(angle);

        cylinder_vertices[2*i + 0] = Vector4f(CYLINDER_RADIUS * sc.second, CYLINDER_RADIUS * sc.first, 0.0f, 1.0f);
        cylinder_vertices[2*i + 1] = Vector4f(CYLINDER_RADIUS * sc.second, CYLINDER_RADIUS * sc.first, CONE_CYLINDER_HEIGHT_RATIO, 1.0f);

        cone_vertices[i+1] = Vector4f(
            CONE_CYLINDER_RADIUS_RATIO * CYLINDER_RADIUS * sc.second,
            CONE_CYLINDER_RADIUS_RATIO * CYLINDER_RADIUS * sc.first,
            CONE_CYLINDER_HEIGHT_RATIO,
            1.0f
        );
    }

    m_cone_shader.bind();
    m_cone_shader.upload_attrib("in_pos", (float*)cone_vertices.data(), 4, cone_vertices.size());
    m_cylinder_shader.bind();
    m_cylinder_shader.upload_attrib("in_pos", (float*)cylinder_vertices.data(), 4, cylinder_vertices.size());
}

TEKARI_NAMESPACE_END