#pragma once

#include <tekari/common.h>
#include <tekari/color_map.h>
#include <tekari/color_map_button.h>
#include <nanogui/window.h>
#include <vector>

TEKARI_NAMESPACE_BEGIN

class ColorMapSelectionWindow : public nanogui::Window
{
public:
    ColorMapSelectionWindow(nanogui::Widget* parent, vector<std::shared_ptr<ColorMap>> color_maps);

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

    void deselect_all_color_maps_button();

    void set_close_callback(function<void()> close_callback)
    {
        m_close_callback = close_callback;
        m_close_button->set_callback(m_close_callback);
    }
    void set_selection_callback(function<void(std::shared_ptr<ColorMap> color_map)> selection_callback) { m_selection_callback = selection_callback; }
    void set_selected_button(size_t index) { m_color_map_buttons[index]->set_selected(true); }

private:
    int color_map_button_index(const ColorMapButton* button) const;

    function<void(void)> m_close_callback;
    function<void(std::shared_ptr<ColorMap>)> m_selection_callback;

    size_t m_selected_color_map_index;
    vector<ColorMapButton*> m_color_map_buttons;

    nanogui::Button* m_close_button;
};

TEKARI_NAMESPACE_END