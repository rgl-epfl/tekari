#pragma once

#include <nanogui/window.h>
#include <string>
#include <iostream>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class HelpWindow : public nanogui::Window {
public:
    HelpWindow(nanogui::Widget* parent, std::function<void()> closeCallback);

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
    virtual void performLayout(NVGcontext *ctx) override;

    static std::string COMMAND;
    static std::string ALT;

private:
    std::function<void()> m_CloseCallback;

    nanogui::VScrollPanel *m_ScrollPanel;
};

TEKARI_NAMESPACE_END