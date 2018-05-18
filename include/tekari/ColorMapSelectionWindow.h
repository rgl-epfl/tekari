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

    void setCloseCallback(std::function<void()> closeCallback)
    {
        mCloseCallback = closeCallback;
        mCloseButton->setCallback(mCloseCallback);
    }
    void setSelectionCallback(std::function<void(std::shared_ptr<ColorMap> colorMap)> selectionCallback) { mSelectionCallback = selectionCallback; }
    void setSelectedButton(size_t index) { mColorMapButtons[index]->setSelected(true); }

private:
    int colorMapButtonIndex(const ColorMapButton* button) const;

    std::function<void(void)> mCloseCallback;
    std::function<void(std::shared_ptr<ColorMap>)> mSelectionCallback;

    unsigned int mSelectedColorMapIndex;
    std::vector<ColorMapButton*> mColorMapButtons;

    nanogui::Button *mCloseButton;
};

TEKARI_NAMESPACE_END