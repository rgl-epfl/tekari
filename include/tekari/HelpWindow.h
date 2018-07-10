#pragma once

#include "common.h"

#include <nanogui/window.h>
#include <string>
#include <iostream>

TEKARI_NAMESPACE_BEGIN

class HelpWindow : public nanogui::Window {
public:
    HelpWindow(nanogui::Widget* parent, std::function<void()> close_callback);

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    virtual void perform_layout(NVGcontext *ctx) override;

    static std::string COMMAND;
    static std::string ALT;

private:
    std::function<void()> m_close_callback;

    nanogui::VScrollPanel *m_scroll_panel;
};

TEKARI_NAMESPACE_END