#pragma once

#include <tekari/common.h>
#include <nanogui/widget.h>
#include <nanogui/opengl.h>
#include <nanogui/button.h>
#include <tekari/ColorMap.h>

TEKARI_NAMESPACE_BEGIN

class ColorMapButton : public nanogui::Widget
{
public:
    ColorMapButton(nanogui::Widget* parent, std::shared_ptr<ColorMap> color_map);
    ~ColorMapButton();

    virtual bool mouse_button_event(const Vector2i& p, int button, bool down, int modifiers) override;
    void draw(NVGcontext* ctx) override;

    void set_callback(function<void(ColorMapButton*)> callback) { m_callback = callback; }

    bool selected() const { return m_selected; }
    void set_selected(bool selected) { m_selected = selected; }

    std::shared_ptr<ColorMap> color_map() { return m_color_map; }

private:
    nanogui::GLShader m_color_map_shader;
    std::shared_ptr<ColorMap> m_color_map;

    function<void(ColorMapButton*)> m_callback;

    bool m_selected;
};

TEKARI_NAMESPACE_END