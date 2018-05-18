#include "tekari/ColorMapButton.h"

#include <nanogui/screen.h>
#include <nanogui/window.h>

using namespace nanogui;

TEKARI_NAMESPACE_BEGIN

ColorMapButton::ColorMapButton(nanogui::Widget * parent, std::shared_ptr<ColorMap> colorMap)
:   Widget(parent)
,   mColorMap(colorMap)
,   mSelected(false)
{
    mColorMapShader.initFromFiles("color_map_viewer", "../resources/shaders/color_map.vert",
        "../resources/shaders/color_map.frag");

    MatrixXu indices(3, 2);
    indices.col(0) << 0, 1, 2;
    indices.col(1) << 2, 3, 1;

    MatrixXf vertices(2, 4);
    vertices.col(0) << 0, 0;
    vertices.col(1) << 1, 0;
    vertices.col(2) << 0, 1;
    vertices.col(3) << 1, 1;

    mColorMapShader.bind();
    mColorMapShader.uploadIndices(indices);
    mColorMapShader.uploadAttrib("in_pos", vertices);
    mColorMapShader.setUniform("color_map", 0);

    setTooltip(mColorMap->name());
}

ColorMapButton::~ColorMapButton()
{
    mColorMapShader.free();
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
        if (mCallback)
        {
            mCallback(this);
            mSelected = true;
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

    mColorMapShader.bind();
    mColorMap->bind();
    mColorMapShader.setUniform("scale", scaleFactor);
    mColorMapShader.setUniform("offset", imagePosition);
    mColorMapShader.drawIndexed(GL_TRIANGLES, 0, 2);

    // draw border
    if (mMouseFocus || mSelected)
    {
        nvgBeginPath(ctx);
        nvgStrokeWidth(ctx, 1);
        nvgRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, mSize.x() - 1, mSize.y() - 1);
        nvgStrokeColor(ctx, Color(1.0f, 1.0f));
        nvgStroke(ctx);
    }
}

TEKARI_NAMESPACE_END