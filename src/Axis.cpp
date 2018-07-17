#include <tekari/Axis.h>

#include <nanogui/glutil.h>
#include <string>
#include <iostream>
#include <tekari/Arrow.h>

TEKARI_NAMESPACE_BEGIN

Axis::Axis(Vector3f origin)
    : m_origin(origin)
{}

void Axis::draw_gl(const Matrix4f& mvp)
{
    Arrow::instance().draw_gl(
            m_origin,
            Vector3f(1, 0.0f, 0.0f),
            0.2f,
            mvp,
            Color(1.0f, 0.0f, 0.0f, 1.0f)
        );

    Arrow::instance().draw_gl(
            m_origin,
            Vector3f(0.0f, 1.0f, 0.0f),
            0.2f,
            mvp,
            Color(0.0f, 1.0f, 0.0f, 1.0f)
        );

    Arrow::instance().draw_gl(
            m_origin,
            Vector3f(0.0f, 0.0f, 1.0f),
            0.2f,
            mvp,
            Color(0.0f, 0.0f, 1.0f, 1.0f)
        );
}

void Axis::set_origin(const Vector3f& new_origin)
{
    m_origin = new_origin;
}

TEKARI_NAMESPACE_END