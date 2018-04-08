#include "ColorMapButton.h"

#include <nanogui\screen.h>
#include <nanogui\window.h>

using namespace nanogui;

ColorMapButton::ColorMapButton(nanogui::Widget * parent, std::shared_ptr<ColorMap> colorMap)
:   Widget(parent)
,   m_ColorMap(colorMap)
,   m_Selected(false)
{
    m_ColorMapShader.initFromFiles("color_map_viewer", "../resources/shaders/color_map.vert",
        "../resources/shaders/color_map.frag");

    MatrixXu indices(3, 2);
    indices.col(0) << 0, 1, 2;
    indices.col(1) << 2, 3, 1;

    MatrixXf vertices(2, 4);
    vertices.col(0) << 0, 0;
    vertices.col(1) << 1, 0;
    vertices.col(2) << 0, 1;
    vertices.col(3) << 1, 1;

    m_ColorMapShader.bind();
    m_ColorMapShader.uploadIndices(indices);
    m_ColorMapShader.uploadAttrib("in_pos", vertices);
    m_ColorMapShader.setUniform("color_map", 0);

    setTooltip(m_ColorMap->name());
}

ColorMapButton::~ColorMapButton()
{
    m_ColorMapShader.free();
}

bool ColorMapButton::mouseButtonEvent(const nanogui::Vector2i & p, int button, bool down, int modifiers)
{
    if (Widget::mouseButtonEvent(p, button, down, modifiers)) {
        return true;
    }

    if (!mEnabled || !down) {
        return false;
    }

    if (button == GLFW_MOUSE_BUTTON_1) {
        if (m_Callback)
        {
            m_Callback(m_ColorMap);
            m_Selected = true;
            return true;
        }
    }

    return false;
}

void ColorMapButton::draw(NVGcontext * ctx)
{
    Widget::draw(ctx);
    nvgEndFrame(ctx); // Flush the NanoVG draw stack, not necessary to call nvgBeginFrame afterwards.

    const Screen* screen = dynamic_cast<const Screen*>(this->window()->parent());
    assert(screen);
    Vector2f screenSize = screen->size().cast<float>();
    Vector2f scaleFactor = mSize.cast<float>().cwiseQuotient(screenSize);
    Vector2f positionInScreen = absolutePosition().cast<float>();
    Vector2f imagePosition = positionInScreen.cwiseQuotient(screenSize);

    m_ColorMapShader.bind();
    m_ColorMap->bind();
    m_ColorMapShader.setUniform("scale", scaleFactor);
    m_ColorMapShader.setUniform("offset", imagePosition);
    m_ColorMapShader.drawIndexed(GL_TRIANGLES, 0, 2);

    // draw border
    if (mMouseFocus || m_Selected)
    {
        nvgBeginPath(ctx);
        nvgStrokeWidth(ctx, 1);
        nvgRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, mSize.x() - 1, mSize.y() - 1);
        nvgStrokeColor(ctx, Color(1.0f, 1.0f));
        nvgStroke(ctx);
    }
}

