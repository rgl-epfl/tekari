#include <tekari/data_sample_button.h>

#include <iostream>

#include <nanogui/opengl.h>
#include <nanogui/common.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/entypo.h>
#include <nanogui/slider.h>
#include <nanogui/button.h>
#include <nanogui/checkbox.h>

using nanogui::Widget;
using nanogui::Window;
using nanogui::Slider;
using nanogui::Button;
using nanogui::GridLayout;
using nanogui::CheckBox;
using nanogui::Alignment;
using nanogui::Orientation;
using nanogui::BoxLayout;
using nanogui::Popup;
using nanogui::Label;

TEKARI_NAMESPACE_BEGIN

DataSampleButton::DataSampleButton(Widget* parent, const string & label, bool is_spectral)
:   Widget{ parent }
,   m_label{ label }
,   m_display_label{ label.size() > 20 ? label.substr(0, 17) + ". .. " : label }
,   m_selected(false)
,   m_visible(true)
,   m_dirty(false)
,   m_toggle_view_button_pos{ 155, 15 }
,   m_delete_button_pos{ 155 + 2*BUTTON_RADIUS + 2, 15 }
,   m_toggle_view_button_hovered(false)
,   m_delete_button_hovered(false)
{
    set_tooltip(m_label);
}

bool DataSampleButton::mouse_button_event(const Vector2i & p, int button, bool down, int modifiers)
{
    if (Widget::mouse_button_event(p, button, down, modifiers)) {
        return true;
    }

    if (!m_enabled || !down) {
        return false;
    }

    if (button == GLFW_MOUSE_BUTTON_1) {
        m_callback();
        if (in_delete_button(p))
        {
            m_delete_callback();
            return true;
        }
        else if (in_toggle_view_button(p))
        {
            toggle_view();
            return true;
        }
    }

    return false;
}

bool DataSampleButton::mouse_enter_event(const Vector2i & p, bool enter)
{
    Widget::mouse_enter_event(p, enter);
    m_delete_button_hovered = false;
    m_toggle_view_button_hovered = false;
    return false;
}

bool DataSampleButton::mouse_motion_event(const Vector2i& p, const Vector2i& rel, int button, int modifiers)
{
    if (Widget::mouse_motion_event(p, rel, button, modifiers)) {
        return true;
    }

    m_delete_button_hovered = in_delete_button(p);
    m_toggle_view_button_hovered = in_toggle_view_button(p);
    return false;
}

void DataSampleButton::draw(NVGcontext* ctx)
{
    Color fill_color =  m_selected ? Color(0.0f, 0.8f, 0.2f, 0.5f) :
                        m_mouse_focus ? m_theme->m_button_gradient_top_focused :
                        m_theme->m_button_gradient_top_unfocused;
    float delete_button_fill_opacity = m_delete_button_hovered ? 0.4f : 0.2f;
    float toggle_view_button_fill_opacity = m_toggle_view_button_hovered ? 0.4f : m_visible ? 0.5f : 0.2f;

    // save current nvg state
    nvgSave(ctx);
    nvgTranslate(ctx, m_pos.x(), m_pos.y());

    // draw background
    nvgBeginPath(ctx);
    nvgRect(ctx, 0, 0, m_size.x(), m_size.y());
    nvgFillColor(ctx, fill_color);
    nvgFill(ctx);
    if (m_mouse_focus)
    {
        nvgStrokeColor(ctx, Color(1.0f, 0.8f));
        nvgStrokeWidth(ctx, 1.0f);
        nvgStroke(ctx);
    }

    // draw label
    string label = m_dirty ? m_display_label + "*" : m_display_label;
    nvgFontSize(ctx, 18.0f);
    nvgFontFace(ctx, "sans");
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, m_theme->m_text_color_shadow);
    nvgText(ctx, 5, m_size.y()* 0.5f - 1.0f, label.c_str(), nullptr);
    nvgFillColor(ctx, m_theme->m_text_color);
    nvgText(ctx, 5, m_size.y()* 0.5f, label.c_str(), nullptr);

    // font settings for icons
    nvgFontSize(ctx, BUTTON_RADIUS* 1.3f);
    nvgFontFace(ctx, "icons");
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    // draw delete button
    auto make_tool_button = [&ctx](float opacity, int icon, const Vector2i& pos) {
        nvgBeginPath(ctx);
        nvgCircle(ctx, pos.x(), pos.y(), BUTTON_RADIUS);
        nvgFillColor(ctx, Color(0.0f, opacity));
        nvgFill(ctx);
        auto icon_data = nanogui::utf8(icon);
        nvgFillColor(ctx, Color(0.0f, 0.8f));
        nvgText(ctx, pos.x(), pos.y() - 1.0f, icon_data.data(), nullptr);
        nvgFillColor(ctx, Color(1.0f, 0.8f));
        nvgText(ctx, pos.x(), pos.y(), icon_data.data(), nullptr);
    };
    make_tool_button(delete_button_fill_opacity, ENTYPO_ICON_CROSS, m_delete_button_pos);
    make_tool_button(toggle_view_button_fill_opacity, m_visible ? ENTYPO_ICON_EYE : ENTYPO_ICON_EYE_WITH_LINE, m_toggle_view_button_pos);

    nvgRestore(ctx);
}

void DataSampleButton::toggle_view()
{
    m_visible = !m_visible;
    m_toggle_view_callback(m_visible);
}

TEKARI_NAMESPACE_END