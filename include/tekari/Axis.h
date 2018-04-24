#pragma once

#include <nanogui/glutil.h>
#include <string>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class Axis
{
public:
    Axis(nanogui::Vector3f origin);
    ~Axis();

    void loadShader();

    void drawGL(const nanogui::Matrix4f& mvp);
    void setOrigin(const nanogui::Vector3f& newOrigin);
private:
    nanogui::Vector3f m_Origin;
    nanogui::GLShader m_Shader;
};

TEKARI_NAMESPACE_END