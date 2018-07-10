#pragma once

#include <nanogui/window.h>
#include <functional>

#include "Metadata.h"

TEKARI_NAMESPACE_BEGIN

class MetadataWindow : public nanogui::Window
{
public:
    MetadataWindow(nanogui::Widget* parent, const Metadata* metadata, std::function<void(void)> close_callback);

    bool keyboard_event(int key, int scancode, int action, int modifiers) override;
private:
    std::function<void(void)> m_close_callback;
};

TEKARI_NAMESPACE_END