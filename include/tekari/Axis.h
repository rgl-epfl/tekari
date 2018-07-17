#pragma once

#include <tekari/common.h>

TEKARI_NAMESPACE_BEGIN

class Axis
{
public:
    Axis(Vector3f origin);

    void draw_gl(const Matrix4f& mvp);
    void set_origin(const Vector3f& new_origin);
private:
    Vector3f m_origin;
};

TEKARI_NAMESPACE_END