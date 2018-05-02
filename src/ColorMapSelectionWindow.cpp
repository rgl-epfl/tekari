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

ColorMapSelectionWindow::ColorMapSelectionWindow(Widget* parent, vector<shared_ptr<ColorMap>> colorMaps)
:   Window{ parent, "Color Maps" }
,   m_SelectedColorMapIndex(0)
{
    setFixedWidth(200);
    m_CloseButton = new Button{ buttonPanel(), "", ENTYPO_ICON_CROSS };
    setLayout(new GroupLayout{});

    new Label{ this, "Available Color Maps :", "sans-bold", 18 };
    auto colorMapsButtonContainer = new Widget{ this };
    colorMapsButtonContainer->setLayout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 2, 2 });

    for (auto colorMap : colorMaps)
    {
        new Label{ colorMapsButtonContainer, colorMap->name() };
        auto colorMapButton = new ColorMapButton{ colorMapsButtonContainer, colorMap };
        colorMapButton->setFixedHeight(10);
        colorMapButton->setCallback([this](ColorMapButton* colorMapButton) {
            deselectAllColorMapsButton();
            m_SelectedColorMapIndex = colorMapButtonIndex(colorMapButton);
            m_SelectionCallback(colorMapButton->colorMap());
        });

        m_ColorMapButtons.push_back(colorMapButton);
    }
}

int ColorMapSelectionWindow::colorMapButtonIndex(const ColorMapButton* button) const
{
    auto buttonIter = find(m_ColorMapButtons.begin(), m_ColorMapButtons.end(), button);
    if (buttonIter == m_ColorMapButtons.end())
        return -1;

    return static_cast<int>(buttonIter - m_ColorMapButtons.begin());
}

bool ColorMapSelectionWindow::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Window::keyboardEvent(key, scancode, action, modifiers)) {
        return true;
    }
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            m_CloseCallback();
            return true;
        case GLFW_KEY_UP: case GLFW_KEY_W:
        case GLFW_KEY_DOWN: case GLFW_KEY_S:
        {
            int increment = (key == GLFW_KEY_UP || key == GLFW_KEY_W) ? -1 : 1;
            deselectAllColorMapsButton();

            int selectedColorMapIndex = m_SelectedColorMapIndex + increment;
            if (selectedColorMapIndex < 0) selectedColorMapIndex += m_ColorMapButtons.size();
            m_SelectedColorMapIndex = selectedColorMapIndex % m_ColorMapButtons.size();
            auto colorMapButton = m_ColorMapButtons[m_SelectedColorMapIndex];
            colorMapButton->setSelected(true);
            m_SelectionCallback(colorMapButton->colorMap());
            return true;
        }
        }
    }

    return false;
}

void ColorMapSelectionWindow::deselectAllColorMapsButton()
{
    for (auto colorMapButton : m_ColorMapButtons)
    {
        colorMapButton->setSelected(false);
    }
}

TEKARI_NAMESPACE_END