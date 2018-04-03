#pragma once

#include <nanogui\glutil.h>
#include <vector>
#include "common.h"

class RadialGrid
{
public:
    RadialGrid();
    ~RadialGrid();

    void draw(const nanogui::Matrix4f& model,
        const nanogui::Matrix4f& view,
        const nanogui::Matrix4f& proj);

    void setVisible(bool visible) { m_Visible = visible; }
    bool visible() const { return m_Visible; }

    const nanogui::Color& color() const { return m_Color; }
    void setColor(const nanogui::Color& newColor) { m_Color = newColor; }
    
private:
    nanogui::GLShader m_Shader;
    unsigned int m_CircleCount;
    unsigned int m_VertexPerCircleCount;
    unsigned int m_LinesCount;
    unsigned int m_VertexPerLineCount;
    nanogui::Color m_Color;

    bool m_Visible;
};