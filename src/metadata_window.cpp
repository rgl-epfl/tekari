#include <tekari/metadata_window.h>

#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/layout.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/label.h>
#include <nanogui/glutil.h>

using nanogui::Button;
using nanogui::VScrollPanel;
using nanogui::GridLayout;
using nanogui::GroupLayout;
using nanogui::Alignment;
using nanogui::Orientation;
using nanogui::Label;

TEKARI_NAMESPACE_BEGIN

MetadataWindow::MetadataWindow(Widget* parent, const Metadata* metadata, function<void(void)> close_callback)
    : Window(parent, "Metadata")
    , m_close_callback(close_callback)
{
    auto close_button = new Button{ button_panel(), "", ENTYPO_ICON_CROSS };
    close_button->set_callback(m_close_callback);

    set_layout(new GroupLayout{});

    auto scroll_container = new VScrollPanel{ this };
    scroll_container->set_fixed_height(300);
    auto container = new Widget{ scroll_container };
    container->set_layout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 15, 2 });

    auto raw_meta = metadata->raw_metadata();
    // Log(Info, "%d\n", raw_meta.size());
    for (const string& line : raw_meta) {
        auto pos = line.find_first_of("\t ");
        if (pos == string::npos)
            continue;

        string title = line.substr(1, pos);
        string value = line.substr(pos + 1, line.length());
        if (value.length() > 100) value = value.substr(0, 100);

        new Label(container, title, "sans-bold", 18);
        new Label(container, value);
    }
}

bool MetadataWindow::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Window::keyboard_event(key, scancode, action, modifiers)) {
        return true;
    }

    if (key == GLFW_KEY_ESCAPE) {
        m_close_callback();
        return true;
    }

    return false;
}

TEKARI_NAMESPACE_END