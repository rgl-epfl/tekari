#pragma once

#include <nanogui/glutil.h>
#include <vector>
#include "common.h"

TEKARI_NAMESPACE_BEGIN

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

    void setVisible(bool visible) { mVisible = visible; }
    bool visible() const { return mVisible; }

    void setShowDegrees(bool showDegrees) { mShowDegrees = showDegrees; }
    bool showDegrees() const { return mShowDegrees; }

    const nanogui::Color& color() const { return mColor; }
    void setColor(const nanogui::Color& newColor) { mColor = newColor; }

private:
    static constexpr unsigned int CIRCLE_COUNT = 10;
    static constexpr unsigned int VERTEX_PER_CIRCLE_COUNT = 100;
    static constexpr unsigned int LINE_COUNT = 18;
    static constexpr unsigned int VERTEX_PER_LINE_COUNT = 2;

    nanogui::GLShader mShader;
    nanogui::Color mColor;
    std::vector<std::pair<std::string, nanogui::Vector3f>> mPhiLabels;
    std::vector<std::pair<std::string, nanogui::Vector3f>> mThetaLabels;
    bool mVisible;
    bool mShowDegrees;
};

TEKARI_NAMESPACE_END