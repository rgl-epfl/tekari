#include <tekari/color_map_button.h>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <tekari_resources.h>

using nanogui::Screen;

TEKARI_NAMESPACE_BEGIN

ColorMapButton::ColorMapButton(Widget* parent, std::shared_ptr<ColorMap> color_map)
:   Widget(parent)
,   m_color_map(color_map)
,   m_selected(false)
{
    m_color_map_shader.init("color_map_viewer", VERTEX_SHADER_STR(color_map), FRAGMENT_SHADER_STR(color_map));

    uint32_t indices[3*2] =
    {
        0, 1, 2,
        2, 3, 1
    };


    float vertices[2*4] =
    {
        0, 0,
        1, 0,
        0, 1,
        1, 1
    };

    m_color_map_shader.bind();
    m_color_map_shader.upload_indices(indices, 3, 2);
    m_color_map_shader.upload_attrib("in_pos", vertices, 2, 4);
    m_color_map_shader.set_uniform("color_map", 0);

    set_tooltip(m_color_map->name());
}

ColorMapButton::~ColorMapButton()
{
    m_color_map_shader.free();
}

bool ColorMapButton::mouse_enter_event(const Vector2i &p, bool enter) {
    Widget::mouse_enter_event(p, enter);
    return true;
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

void ColorMapButton::draw(NVGcontext* ctx)
{
    Widget::draw(ctx);
    nvgEndFrame(ctx); // Flush the Nano_v_g draw stack, not necessary to call nvg_begin_frame afterwards.

    const Screen* screen = dynamic_cast<const Screen*>(this->window()->parent());
    assert(screen);
    Vector2f screen_size = Vector2f(screen->size());
    Vector2f scale_factor = Vector2f(m_size) / screen_size;
    Vector2f position_in_screen = Vector2f(absolute_position());
    Vector2f image_position = position_in_screen / screen_size;

    m_color_map_shader.bind();
    m_color_map->bind();
    m_color_map_shader.set_uniform("scale", scale_factor);
    m_color_map_shader.set_uniform("offset", image_position);
    m_color_map_shader.draw_indexed(GL_TRIANGLES, 0, 2);

    // draw border
    if (m_mouse_focus || m_selected)
    {
        nvgBeginPath(ctx);
        nvgStrokeWidth(ctx, 1);
        nvgRect(ctx, m_pos.x() + 0.5f, m_pos.y() + 0.5f, m_size.x() - 1, m_size.y() - 1);
        nvgStrokeColor(ctx, Color(1.0f, 1.0f));
        nvgStroke(ctx);
    }
}

TEKARI_NAMESPACE_END