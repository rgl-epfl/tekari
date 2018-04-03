#pragma once

#include <nanogui/window.h>

#include <string>
#include <iostream>

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