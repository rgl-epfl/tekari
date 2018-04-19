#pragma once

#include <nanogui/glutil.h>
#include <string>

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