#pragma once

#include <tekari/common.h>
#include <nanogui/widget.h>

TEKARI_NAMESPACE_BEGIN

class Slider2D: public nanogui::Widget
{
public:
    Slider2D(Widget *parent);

    Vector2f value() const { return m_value; }
    void set_value(Vector2f value) { m_value = value; }

    const Color &highlight_color() const { return m_highlight_color; }
    void set_highlight_color(const Color &highlight_color) { m_highlight_color = highlight_color; }

    std::pair<Vector2f, Vector2f> range() const { return m_range; }
    void set_range(std::pair<Vector2f, Vector2f> range) { m_range = range; }

    std::function<void(Vector2f)> callback() const { return m_callback; }
    void set_callback(const std::function<void(Vector2f)> &callback) { m_callback = callback; }

    std::function<void(Vector2f)> final_callback() const { return m_final_callback; }
    void set_final_callback(const std::function<void(Vector2f)> &callback) { m_final_callback = callback; }

    virtual Vector2i preferred_size(NVGcontext *ctx) const override;
    virtual bool mouse_drag_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;
    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override;
    virtual void draw(NVGcontext* ctx) override;

protected:
    Vector2f m_value;
    std::function<void(Vector2f)> m_callback;
    std::function<void(Vector2f)> m_final_callback;
    std::pair<Vector2f, Vector2f> m_range;
    Color m_highlight_color;
};

TEKARI_NAMESPACE_END