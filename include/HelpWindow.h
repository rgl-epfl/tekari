#pragma once

#include <nanogui/window.h>

#include <string>

class HelpWindow : public nanogui::Window {
public:
    HelpWindow(nanogui::Widget* parent, std::function<void()> closeCallback);

    bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    static std::string COMMAND;
    static std::string ALT;

private:
    std::function<void()> m_CloseCallback;
};