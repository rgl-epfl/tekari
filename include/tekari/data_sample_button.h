#pragma once

#include <tekari/data_sample.h>
#include <nanogui/widget.h>
#include <nanogui/popup.h>

TEKARI_NAMESPACE_BEGIN

class DataSampleButton : public nanogui::Widget
{
public:
    DataSampleButton(nanogui::Widget* parent, const string& label, bool is_spectral);

    virtual bool mouse_button_event(const Vector2i& p, int button, bool down, int modifiers) override;
    virtual bool mouse_enter_event(const Vector2i& p, bool enter) override;
    virtual bool mouse_motion_event(const Vector2i& p, const Vector2i& rel, int button, int modifiers) override;
    virtual void draw(NVGcontext* ctx) override;
    virtual void perform_layout(NVGcontext* ctx) override;

    void toggle_view();

    bool selected() const { return m_selected; }
    void set_selected(bool selected) { m_selected = selected; }
    void set_dirty(bool dirty) { m_dirty = dirty; }

    void set_callback             (function<void(void)> callback)         { m_callback = callback; }
    void set_delete_callback      (function<void(void)> callback)         { m_delete_callback = callback; }
    void set_toggle_view_callback (function<void(bool)> callback)         { m_toggle_view_callback = callback; }

    void set_display_as_log_callback(function<void(bool)> callback);
    void set_view_toggles_callback(function<void(bool)> callback);
  
    void show_popup(bool visible) { m_popup->set_visible(visible); }
    void remove_popup_from_parent() { m_popup->parent()->remove_child(m_popup); }

    void toggle_view(DataSample::Views view, bool check);
    bool is_view_toggled(DataSample::Views view);

    void toggle_log_view();

private:
    bool In_toggle_view_button(const Vector2i& p) const {
        return squared_norm(p - m_pos - m_toggle_view_button_pos) <= BUTTON_RADIUS*BUTTON_RADIUS;
    }
    bool In_delete_button(const Vector2i& p) const {
        return squared_norm(p - m_pos - m_delete_button_pos) <= BUTTON_RADIUS*BUTTON_RADIUS;
    }

    static constexpr float BUTTON_RADIUS = 10.0f;

    string m_label;
    string m_display_label;
    bool m_selected;
    bool m_visible;
    bool m_dirty;

    Vector2i m_toggle_view_button_pos;
    Vector2i m_delete_button_pos;
    bool m_toggle_view_button_hovered;
    bool m_delete_button_hovered;

    function<void(void)> m_callback;
    function<void(bool)> m_toggle_view_callback;
    function<void(void)> m_delete_callback;

    //std::shared_ptr<DataSample> m_data_sample;
    nanogui::Popup* m_popup;
    nanogui::CheckBox* m_display_as_log;
    nanogui::Button* m_view_toggles[DataSample::Views::VIEW_COUNT];
};

TEKARI_NAMESPACE_END