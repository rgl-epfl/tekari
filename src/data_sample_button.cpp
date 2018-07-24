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

DataSampleButton::DataSampleButton(Widget* parent, const string & label, bool is_spectral, unsigned int max_wave_length_index)
:   Widget{ parent }
,   m_label{ label }
,   m_display_label{ label.size() > 20 ? label.substr(0, 17) + "..." : label }
,    m_selected(false)
,   m_visible(true)
,    m_dirty(false)
,   m_toggle_view_button_pos{ 155, 15 }
,   m_delete_button_pos{ 155 + 2*BUTTON_RADIUS + 2, 15 }
,   m_toggle_view_button_hovered(false)
,   m_delete_button_hovered(false)
{
    set_tooltip(m_label);

    Window* parent_window = window();

    m_popup = new Popup{ parent_window->parent(), window() };
    m_popup->set_visible(false);
    m_popup->set_layout(new BoxLayout{ Orientation::Vertical, Alignment::Fill, 5, 5 });

    new Label{ m_popup, "View Modes" , "sans-bold", 18};
    m_display_as_log = new CheckBox{ m_popup, "Display as log" };

    auto button_container = new Widget{ m_popup };
    button_container->set_layout(new GridLayout{ Orientation::Horizontal, 4, Alignment::Fill });

    auto make_view_button = [button_container](const string& label, const string& tooltip, bool pushed) {
        auto button = new Button(button_container, label);
        button->set_flags(Button::Flags::ToggleButton);
        button->set_tooltip(tooltip);
        button->set_pushed(pushed);
        return button;
    };
    m_view_toggles[DataSample::Views::MESH]   = make_view_button("Mesh", "Show/Hide mesh for this data sample (M)", true);
    m_view_toggles[DataSample::Views::PATH]   = make_view_button("Path", "Show/Hide path for this data sample (P)", false);
    m_view_toggles[DataSample::Views::POINTS] = make_view_button("Points", "Toggle points view for this data sample (Shift + P)", false);
    m_view_toggles[DataSample::Views::INCIDENT_ANGLE] = make_view_button("Incident Angle", "Show/Hide incident angle for this data sample (Shift + I)", true);

    if (is_spectral)
    {
        auto wave_length_index_label = new Label{ m_popup, "Wave length index : 0", "sans-bold", 18 };
        auto wave_length_slider = new Slider{ m_popup };
        wave_length_slider->set_range(make_pair(0, max_wave_length_index));
        wave_length_slider->set_callback([this, wave_length_slider, wave_length_index_label](float value) {
            unsigned int wave_length_index = static_cast<unsigned int>(round(value));
            wave_length_slider->set_value(wave_length_index);
            wave_length_index_label->set_caption("Wave length index : " + to_string(wave_length_index));
            m_wave_length_slider_callback(wave_length_index);
        });
    }
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
        if (In_delete_button(p))
        {
            m_delete_callback();
            return true;
        }
        else if (In_toggle_view_button(p))
        {
            toggle_view();
            return true;
        }
        else if (m_callback)
        {
            m_popup->set_visible(!m_popup->visible());
            m_callback();
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

    m_delete_button_hovered = In_delete_button(p);
    m_toggle_view_button_hovered = In_toggle_view_button(p);
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

void DataSampleButton::perform_layout(NVGcontext* ctx)
{
    Widget::perform_layout(ctx);

    const Window* parent_window = window();

    int pos_y = absolute_position().y() - parent_window->position().y() + m_size.y() / 2;
    if (m_popup->side() == Popup::Right)
        m_popup->set_anchor_pos(Vector2i(parent_window->width() + 15, pos_y));
    else
        m_popup->set_anchor_pos(Vector2i(0 - 15, pos_y));
}

void DataSampleButton::toggle_view()
{
    m_visible = !m_visible;
    m_toggle_view_callback(m_visible);
}

void DataSampleButton::toggle_view(DataSample::Views view, bool check)
{
    m_view_toggles[static_cast<int>(view)]->set_pushed(check);
}

bool DataSampleButton::is_view_toggled(DataSample::Views view)
{
    return m_view_toggles[static_cast<int>(view)]->pushed();
}

void DataSampleButton::toggle_log_view()
{
    m_display_as_log->set_checked(!m_display_as_log->checked());
}

void DataSampleButton::set_view_toggles_callback(function<void(bool)> callback) {
    for (int i = 0; i != DataSample::Views::VIEW_COUNT; ++i)
    {
        DataSample::Views view = static_cast<DataSample::Views>(i);
        m_view_toggles[view]->set_change_callback(callback);
    }
}

void DataSampleButton::set_display_as_log_callback(function<void(bool)> callback)
{
    m_display_as_log->set_callback(callback);
}

TEKARI_NAMESPACE_END