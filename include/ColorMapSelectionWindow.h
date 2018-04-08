#pragma once

#include <nanogui\window.h>
#include <memory>
#include <vector>

#include "ColorMap.h"
#include "ColorMapButton.h"

class ColorMapSelectionWindow : public nanogui::Window
{
public:
    ColorMapSelectionWindow(nanogui::Widget* parent, std::vector<std::shared_ptr<ColorMap>> colorMaps, std::function<void()> closeCallback,
        std::function<void(std::shared_ptr<ColorMap> colorMap)> selectionCallback);

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
    virtual void performLayout(NVGcontext *ctx) override;

    void deselectAllColorMapsButton();

    void setSelectedButton(size_t index) { m_ColorMapButtons[index]->setSelected(true); }

private:
    std::function<void(void)> m_CloseCallback;
    std::function<void(std::shared_ptr<ColorMap>)> m_SelectionCallback;

    std::vector<ColorMapButton*> m_ColorMapButtons;
};