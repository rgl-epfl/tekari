#pragma once

#include "common.h"

#include <nanogui/window.h>
#include <memory>
#include <vector>

#include "ColorMap.h"
#include "ColorMapButton.h"

TEKARI_NAMESPACE_BEGIN

class ColorMapSelectionWindow : public nanogui::Window
{
public:
    ColorMapSelectionWindow(nanogui::Widget* parent, std::vector<std::shared_ptr<ColorMap>> color_maps);

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;

    void deselect_all_color_maps_button();

    void set_close_callback(std::function<void()> close_callback)
    {
        m_close_callback = close_callback;
        m_close_button->set_callback(m_close_callback);
    }
    void set_selection_callback(std::function<void(std::shared_ptr<ColorMap> color_map)> selection_callback) { m_selection_callback = selection_callback; }
    void set_selected_button(size_t index) { m_color_map_buttons[index]->set_selected(true); }

private:
    int color_map_button_index(const ColorMapButton* button) const;

    std::function<void(void)> m_close_callback;
    std::function<void(std::shared_ptr<ColorMap>)> m_selection_callback;

    unsigned int m_selected_color_map_index;
    std::vector<ColorMapButton*> m_color_map_buttons;

    nanogui::Button *m_close_button;
};

TEKARI_NAMESPACE_END