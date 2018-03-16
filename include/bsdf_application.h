#pragma once

#include <nanogui/nanogui.h>
#include "bsdf_canvas.h"

class BSDFApplication : public nanogui::Screen {
public:
    BSDFApplication();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void drawContents() override;
    void requestLayoutUpdate() { m_RequiresLayoutUpdate = true; }

private:
    void updateLayout();

    bool m_RequiresLayoutUpdate = false;


    nanogui::Widget* m_VerticalScreenSplit;

    nanogui::Widget* m_Sidebar;
    nanogui::Button* m_HelpButton;
    nanogui::Widget* m_SidebarLayout;

    BSDFCanvas *m_BSDFCanvas;


    bool showNormal;
    bool showLog;
    bool showSensorPath;
    bool showPointHeights;

    std::string fileName;
    std::string imageName;
};