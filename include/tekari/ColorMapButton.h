#pragma once

#include <nanogui/widget.h>
#include <nanogui/opengl.h>
#include <nanogui/button.h>
#include <functional>
#include <memory>

#include "common.h"
#include "ColorMap.h"

TEKARI_NAMESPACE_BEGIN

class ColorMapButton : public nanogui::Widget
{
public:
    ColorMapButton(nanogui::Widget* parent, std::shared_ptr<ColorMap> colorMap);
    ~ColorMapButton();

    virtual bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;
    void draw(NVGcontext *ctx) override;

    void setCallback(std::function<void(ColorMapButton*)> callback) { mCallback = callback; }

    bool selected() const { return mSelected; }
    void setSelected(bool selected) { mSelected = selected; }

    std::shared_ptr<ColorMap> colorMap() { return mColorMap; }

private:
    nanogui::GLShader mColorMapShader;
    std::shared_ptr<ColorMap> mColorMap;

    std::function<void(ColorMapButton*)> mCallback;

    bool mSelected;
};

TEKARI_NAMESPACE_END