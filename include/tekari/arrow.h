#pragma once

#include <tekari/common.h>
#include <nanogui/glutil.h>

TEKARI_NAMESPACE_BEGIN

class Arrow
{
public:

    static Arrow& instance();

    ~Arrow();

    void draw_gl(   const Vector3f& origin,
                    const Vector3f& direction,
                    float scale,
                    const Matrix4f& vp,
                    const Color& color);
    void load_shaders();

private:
    Arrow()    {}

    nanogui::GLShader m_cone_shader;
    nanogui::GLShader m_cylinder_shader;
};

TEKARI_NAMESPACE_END