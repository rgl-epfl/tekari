#pragma once

#include "DataSample.h"

#include <nanogui/widget.h>
#include <nanogui/popup.h>
#include <functional>
#include <memory>

TEKARI_NAMESPACE_BEGIN

class DataSampleButton : public nanogui::Widget
{
public:
    DataSampleButton(nanogui::Widget* parent, const std::string &label, bool is_spectral, unsigned int max_wave_length_index);

    virtual bool mouse_button_event(const Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool mouse_enter_event(const Vector2i &p, bool enter) override;
    virtual bool mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;
    virtual void perform_layout(NVGcontext *ctx) override;

    void toggle_view();

    bool selected() const { return m_selected; }
    void set_selected(bool selected) { m_selected = selected; }
    void set_dirty(bool dirty) { m_dirty = dirty; }

    void set_callback                    (std::function<void(void)> callback)         { m_callback = callback; }
    void set_delete_callback                (std::function<void(void)> callback)         { m_delete_callback = callback; }
    void set_toggle_view_callback            (std::function<void(bool)> callback)         { m_toggle_view_callback = callback; }
    void set_wave_length_slider_callback    (std::function<void(unsigned int)> callback) { m_wave_length_slider_callback = callback; }

    void set_display_as_log_callback(std::function<void(bool)> callback);
    void set_view_toggles_callback(std::function<void(bool)> callback);
  
    void show_popup(bool visible) { m_popup->set_visible(visible); }
    void remove_popup_from_parent() { m_popup->parent()->remove_child(m_popup); }

    void toggle_view(DataSample::Views view, bool check);
    bool is_view_toggled(DataSample::Views view);

    void toggle_log_view();

private:
    bool In_toggle_view_button(const Vector2i& p) const {
        return (p - m_pos - m_toggle_view_button_pos).squared_norm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }
    bool In_delete_button(const Vector2i& p) const {
        return (p - m_pos - m_delete_button_pos).squared_norm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }

    static constexpr float BUTTON_RADIUS = 10.0f;

    std::string m_label;
    std::string m_display_label;
    bool m_selected;
    bool m_visible;
    bool m_dirty;

    Vector2i m_toggle_view_button_pos;
    Vector2i m_delete_button_pos;
    bool m_toggle_view_button_hovered;
    bool m_delete_button_hovered;

    std::function<void(void)> m_callback;
    std::function<void(bool)> m_toggle_view_callback;
    std::function<void(void)> m_delete_callback;
    std::function<void(unsigned int)> m_wave_length_slider_callback;

    //std::shared_ptr<DataSample> m_data_sample;
    nanogui::Popup *m_popup;
    nanogui::CheckBox* m_display_as_log;
    nanogui::Button* m_view_toggles[DataSample::Views::VIEW_COUNT];
};

TEKARI_NAMESPACE_END