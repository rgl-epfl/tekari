#pragma once

#include <nanogui/nanogui.h>
#include "bsdf_canvas.h"

class BSDFApplication : public nanogui::Screen {
public:
    BSDFApplication();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void drawContents() override;
    void requestLayoutUpdate() { m_RequiresLayoutUpdate = true; }

    void openDataSampleDialog();
    void saveScreenShot();
    void closeSelectedDataSample();

private:
    void updateLayout();

    bool m_RequiresLayoutUpdate = false;


    nanogui::Window* m_ToolWindow;
    nanogui::Button* m_HelpButton;

    BSDFCanvas *m_BSDFCanvas;


    bool showNormal;
    bool showLog;
    bool showSensorPath;
    bool showPointHeights;

    std::string fileName;
    std::string imageName;
};