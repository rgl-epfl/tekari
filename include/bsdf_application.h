#pragma once

#include <nanogui/nanogui.h>
#include "bsdf_canvas.h"

class BSDFApplication : public nanogui::Screen {
public:
    BSDFApplication();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;

private:
    BSDFCanvas *mCanvas;

    bool showNormal;
    bool showLog;
    bool showSensorPath;
    bool showPointHeights;

    std::string fileName;
    std::string imageName;
};