#pragma once

#include <nanogui/glutil.h>
#include <vector>
#include "common.h"

class RadialGrid
{
public:
    RadialGrid();
    ~RadialGrid();

    void drawGL(
        const nanogui::Matrix4f& model,
        const nanogui::Matrix4f& view,
        const nanogui::Matrix4f& proj);

    void draw(NVGcontext *ctx,
        const nanogui::Vector2i& canvasSize,
        const nanogui::Matrix4f& model,
        const nanogui::Matrix4f& view,
        const nanogui::Matrix4f& proj);

    void setVisible(bool visible) { m_Visible = visible; }
    bool visible() const { return m_Visible; }

    void setShowDegrees(bool showDegrees) { m_ShowDegrees = showDegrees; }
    bool showDegrees() const { return m_ShowDegrees; }

    const nanogui::Color& color() const { return m_Color; }
    void setColor(const nanogui::Color& newColor) { m_Color = newColor; }

private:
    static constexpr unsigned int CIRCLE_COUNT = 10;
    static constexpr unsigned int VERTEX_PER_CIRCLE_COUNT = 100;
    static constexpr unsigned int LINE_COUNT = 18;
    static constexpr unsigned int VERTEX_PER_LINE_COUNT = 2;

    nanogui::GLShader m_Shader;
    nanogui::Color m_Color;
    std::vector<std::pair<std::string, nanogui::Vector3f>> m_DegreesLabel;
    bool m_Visible;
    bool m_ShowDegrees;
};