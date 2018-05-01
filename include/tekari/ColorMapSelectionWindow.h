#pragma once

#include <nanogui/window.h>
#include <memory>
#include <vector>

#include "common.h"
#include "ColorMap.h"
#include "ColorMapButton.h"

TEKARI_NAMESPACE_BEGIN

class ColorMapSelectionWindow : public nanogui::Window
{
public:
    ColorMapSelectionWindow(nanogui::Widget* parent, std::vector<std::shared_ptr<ColorMap>> colorMaps);

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void deselectAllColorMapsButton();

    void setCloseCallback(std::function<void()> closeCallback) { m_CloseCallback = closeCallback; }
    void setSelectionCallback(std::function<void(std::shared_ptr<ColorMap> colorMap)> selectionCallback) { m_SelectionCallback = selectionCallback; }
    void setSelectedButton(size_t index) { m_ColorMapButtons[index]->setSelected(true); }

private:
    int colorMapButtonIndex(const ColorMapButton* button) const;

    std::function<void(void)> m_CloseCallback;
    std::function<void(std::shared_ptr<ColorMap>)> m_SelectionCallback;

    unsigned int m_SelectedColorMapIndex;
    std::vector<std::shared_ptr<ColorMap>> m_ColorMaps;
    std::vector<ColorMapButton*> m_ColorMapButtons;
};

TEKARI_NAMESPACE_END