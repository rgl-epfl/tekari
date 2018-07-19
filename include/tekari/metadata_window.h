#pragma once

#include <nanogui/window.h>
#include <tekari/metadata.h>

TEKARI_NAMESPACE_BEGIN

class MetadataWindow : public nanogui::Window
{
public:
    MetadataWindow(nanogui::Widget* parent, const Metadata* metadata, function<void(void)> close_callback);

    bool keyboard_event(int key, int scancode, int action, int modifiers) override;
private:
    function<void(void)> m_close_callback;
};

TEKARI_NAMESPACE_END