#pragma once

#include <nanogui/glutil.h>
#include <string>

class Axis
{
public:
    Axis(nanogui::Vector3f origin);
    ~Axis();

    void drawGL(
        const nanogui::Matrix4f& model,
        const nanogui::Matrix4f& view,
        const nanogui::Matrix4f& proj);
    void setOrigin(nanogui::Vector3f newOrigin);
private:
    nanogui::Vector3f m_Origin;
    nanogui::GLShader m_Shader;
};