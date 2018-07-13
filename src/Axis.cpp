#include "tekari/Axis.h"

#include <nanogui/glutil.h>
#include <string>
#include <iostream>

TEKARI_NAMESPACE_BEGIN

using namespace nanogui;

Axis::Axis(Vector3f origin)
    : m_origin(origin)
{}
Axis::~Axis()
{
    m_shader.free();
}

void Axis::load_shader()
{
    const std::string shader_path = "../resources/shaders/";
    m_shader.init_from_files("axis", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");
    m_shader.bind();
    m_shader.set_uniform("length", 0.15f);
    m_shader.upload_attrib("pos", Vector3f{ 0, 0, 0 }.data(), 3, 1);
    m_shader.set_uniform("origin", m_origin);
}

void Axis::draw_gl(const Matrix4f& mvp)
{
    glEnable(GL_DEPTH_TEST);
    m_shader.bind();
    m_shader.set_uniform("origin", m_origin);
    m_shader.set_uniform("model_view_proj", mvp);

    m_shader.set_uniform("direction", Vector3f{ 1, 0, 0 });
    m_shader.set_uniform("color", Vector3f{ 1, 0, 0 });
    m_shader.draw_array(GL_POINTS, 0, 1);

    m_shader.set_uniform("direction", Vector3f{ 0, 1, 0 });
    m_shader.set_uniform("color", Vector3f{ 0, 1, 0 });
    m_shader.draw_array(GL_POINTS, 0, 1);

    m_shader.set_uniform("direction", Vector3f{ 0, 0, 1 });
    m_shader.set_uniform("color", Vector3f{ 0, 0, 1 });
    m_shader.draw_array(GL_POINTS, 0, 1);
    glDisable(GL_DEPTH_TEST);
}

void Axis::set_origin(const Vector3f& new_origin)
{
    m_origin = new_origin;
}

TEKARI_NAMESPACE_END