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

ColorMapSelectionWindow::ColorMapSelectionWindow(Widget* parent, vector<shared_ptr<ColorMap>> colorMaps,
    function<void()> closeCallback, function<void(shared_ptr<ColorMap> colorMap)> selectionCallback)
:   Window{ parent, "Color Maps" }
,   m_CloseCallback{ closeCallback }
,   m_SelectionCallback(selectionCallback)
{
    setFixedWidth(200);
    auto closeButton = new Button{ buttonPanel(), "", ENTYPO_ICON_CROSS };
    closeButton->setCallback(m_CloseCallback);
    setLayout(new GroupLayout{});

    new Label{ this, "Available Color Maps :", "sans-bold", 18 };
    auto colorMapsButtonContainer = new Widget{ this };
    colorMapsButtonContainer->setLayout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 2, 2 });

    for (auto colorMap : colorMaps)
    {
        auto colorMapName = new Label{ colorMapsButtonContainer, colorMap->name() };
        //colorMapName->setFixedHeight(10);
        auto colorMapButton = new ColorMapButton{ colorMapsButtonContainer, colorMap };
        colorMapButton->setFixedHeight(10);
        colorMapButton->setCallback([this](shared_ptr<ColorMap> colorMap) {
            deselectAllColorMapsButton();
            m_SelectionCallback(colorMap);
        });

        m_ColorMapButtons.push_back(colorMapButton);
    }
}

bool ColorMapSelectionWindow::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Window::keyboardEvent(key, scancode, action, modifiers)) {
        return true;
    }

    if (key == GLFW_KEY_ESCAPE) {
        m_CloseCallback();
        return true;
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