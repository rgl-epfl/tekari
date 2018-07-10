#include "tekari/ColorMapButton.h"

#include <nanogui/screen.h>
#include <nanogui/window.h>

using namespace nanogui;

TEKARI_NAMESPACE_BEGIN

ColorMapButton::ColorMapButton(Widget * parent, std::shared_ptr<ColorMap> color_map)
:   Widget(parent)
,   m_color_map(color_map)
,   m_selected(false)
{
    m_color_map_shader.init_from_files("color_map_viewer", "../resources/shaders/color_map.vert",
        "../resources/shaders/color_map.frag");

    MatrixXu indices(3, 2);
    indices.col(0) << 0, 1, 2;
    indices.col(1) << 2, 3, 1;

    MatrixXf vertices(2, 4);
    vertices.col(0) << 0, 0;
    vertices.col(1) << 1, 0;
    vertices.col(2) << 0, 1;
    vertices.col(3) << 1, 1;

    m_color_map_shader.bind();
    m_color_map_shader.upload_indices(indices);
    m_color_map_shader.upload_attrib("in_pos", vertices);
    m_color_map_shader.set_uniform("color_map", 0);

    set_tooltip(m_color_map->name());
}

ColorMapButton::~ColorMapButton()
{
    m_color_map_shader.free();
}

bool ColorMapButton::mouse_button_event(const Vector2i & p, int button, bool down, int modifiers)
{
    if (Widget::mouse_button_event(p, button, down, modifiers)) {
        return true;
    }

    if (!m_enabled || !down) {
        return false;
    }

    if (button == GLFW_MOUSE_BUTTON_1) {
        if (m_callback)
        {
            m_callback(this);
            m_selected = true;
            return true;
        }
    }

    return false;
}

void ColorMapButton::draw(NVGcontext * ctx)
{
    Widget::draw(ctx);
    nvg_end_frame(ctx); // Flush the Nano_v_g draw stack, not necessary to call nvg_begin_frame afterwards.

    const Screen* screen = dynamic_cast<const Screen*>(this->window()->parent());
    assert(screen);
    Vector2f screen_size = screen->size().cast<float>();
    Vector2f scale_factor = m_size.cast<float>().cwise_quotient(screen_size);
    Vector2f position_in_screen = absolute_position().cast<float>();
    Vector2f image_position = position_in_screen.cwise_quotient(screen_size);

    m_color_map_shader.bind();
    m_color_map->bind();
    m_color_map_shader.set_uniform("scale", scale_factor);
    m_color_map_shader.set_uniform("offset", image_position);
    m_color_map_shader.draw_indexed(GL_TRIANGLES, 0, 2);

    // draw border
    if (m_mouse_focus || m_selected)
    {
        nvg_begin_path(ctx);
        nvg_stroke_width(ctx, 1);
        nvg_rect(ctx, m_pos.x() + 0.5f, m_pos.y() + 0.5f, m_size.x() - 1, m_size.y() - 1);
        nvg_stroke_color(ctx, Color(1.0f, 1.0f));
        nvg_stroke(ctx);
    }
}

TEKARI_NAMESPACE_END