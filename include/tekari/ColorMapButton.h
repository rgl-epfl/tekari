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

    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    void draw(NVGcontext *ctx) override;

    void setCallback(std::function<void(ColorMapButton*)> callback) { m_Callback = callback; }

    bool selected() const { return m_Selected; }
    void setSelected(bool selected) { m_Selected = selected; }

    std::shared_ptr<ColorMap> colorMap() { return m_ColorMap; }

private:
    nanogui::GLShader m_ColorMapShader;
    std::shared_ptr<ColorMap> m_ColorMap;

    std::function<void(ColorMapButton*)> m_Callback;

    bool m_Selected;
};

TEKARI_NAMESPACE_END