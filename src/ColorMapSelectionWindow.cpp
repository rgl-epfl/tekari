#include "tekari/ColorMapSelectionWindow.h"

#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/tabwidget.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/window.h>

using namespace nanogui;
using namespace std;

TEKARI_NAMESPACE_BEGIN

ColorMapSelectionWindow::ColorMapSelectionWindow(Widget* parent, vector<shared_ptr<ColorMap>> color_maps)
:   Window{ parent, "Color Maps" }
,   m_selected_color_map_index(0)
{
    set_fixed_width(200);
    m_close_button = new Button{ button_panel(), "", ENTYPO_ICON_CROSS };
    set_layout(new GroupLayout{});

    new Label{ this, "Available Color Maps :", "sans-bold" };
    auto color_maps_button_container = new Widget{ this };
    color_maps_button_container->set_layout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 2, 2 });

    for (auto color_map : color_maps)
    {
        new Label{ color_maps_button_container, color_map->name() };
        auto color_map_button = new ColorMapButton{ color_maps_button_container, color_map };
        color_map_button->set_fixed_size(Vector2i{ 40, 10 });
        color_map_button->set_callback([this](ColorMapButton* color_map_button) {
            deselect_all_color_maps_button();
            m_selected_color_map_index = color_map_button_index(color_map_button);
            m_selection_callback(color_map_button->color_map());
        });

        m_color_map_buttons.push_back(color_map_button);
    }
}

int ColorMapSelectionWindow::color_map_button_index(const ColorMapButton* button) const
{
    auto button_iter = find(m_color_map_buttons.begin(), m_color_map_buttons.end(), button);
    if (button_iter == m_color_map_buttons.end())
        return -1;

    return static_cast<int>(button_iter - m_color_map_buttons.begin());
}

bool ColorMapSelectionWindow::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Window::keyboard_event(key, scancode, action, modifiers)) {
        return true;
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            m_close_callback();
            return true;
        case GLFW_KEY_UP: case GLFW_KEY_W:
        case GLFW_KEY_DOWN: case GLFW_KEY_S:
        {
            int increment = (key == GLFW_KEY_UP || key == GLFW_KEY_W) ? -1 : 1;
            deselect_all_color_maps_button();

            int selected_color_map_index = m_selected_color_map_index + increment;
            if (selected_color_map_index < 0)
                selected_color_map_index += m_color_map_buttons.size();

            m_selected_color_map_index = selected_color_map_index % m_color_map_buttons.size();
            auto color_map_button = m_color_map_buttons[m_selected_color_map_index];
            color_map_button->set_selected(true);
            m_selection_callback(color_map_button->color_map());
            return true;
        }
        }
    }
    return false;
}

void ColorMapSelectionWindow::deselect_all_color_maps_button()
{
    for (auto color_map_button : m_color_map_buttons)
    {
        color_map_button->set_selected(false);
    }
}

TEKARI_NAMESPACE_END