#pragma once

#include <nanogui/glutil.h>
#include <string>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class Axis
{
public:
    Axis(Vector3f origin);
    ~Axis();

    void loadShader();

    void drawGL(const Matrix4f& mvp);
    void setOrigin(const Vector3f& newOrigin);
private:
    Vector3f mOrigin;
    nanogui::GLShader mShader;
};

TEKARI_NAMESPACE_END