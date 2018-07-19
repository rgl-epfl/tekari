#pragma once

#include <tekari/common.h>
#include <nanogui/window.h>

#include <iostream>

TEKARI_NAMESPACE_BEGIN

class HelpWindow : public nanogui::Window {
public:
    HelpWindow(nanogui::Widget* parent, function<void()> close_callback);

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    virtual void perform_layout(NVGcontext* ctx) override;

    static string COMMAND;
    static string ALT;

private:
    function<void()> m_close_callback;

    nanogui::VScrollPanel* m_scroll_panel;
};

TEKARI_NAMESPACE_END