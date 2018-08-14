#include <tekari/help_window.h>

#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/tabwidget.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/window.h>

using nanogui::Button;
using nanogui::VScrollPanel;
using nanogui::GridLayout;
using nanogui::GroupLayout;
using nanogui::BoxLayout;
using nanogui::Alignment;
using nanogui::Orientation;
using nanogui::TabWidget;
using nanogui::Label;

TEKARI_NAMESPACE_BEGIN

#ifdef __APPLE__
string HelpWindow::COMMAND = "Cmd";
#else
string HelpWindow::COMMAND = "Ctrl";
#endif

#ifdef __APPLE__
string HelpWindow::ALT = "Opt";
#else
string HelpWindow::ALT = "Alt";
#endif


HelpWindow::HelpWindow(Widget* parent, function<void()> close_callback)
:   Window{ parent, "Help" }
,   m_close_callback{ close_callback }
{
    set_fixed_width(600);
    auto close_button = new Button{ button_panel(), "", ENTYPO_ICON_CROSS };
    close_button->set_callback(m_close_callback);
    set_layout(new GroupLayout{});

    TabWidget* tab_widget = new TabWidget{ this };

    // Keybindings tab
    {
        Widget* shortcuts = tab_widget->create_tab("Keybindings");
        shortcuts->set_layout(new GroupLayout{});

        m_scroll_panel = new VScrollPanel{ shortcuts };
        auto scroll_content = new Widget{ m_scroll_panel };
        scroll_content->set_layout(new GroupLayout{});
        m_scroll_panel->set_fixed_height(500);

        auto add_shortcut_section = [&scroll_content](const string& label, const string& tooltip="") {
            auto l = new Label{ scroll_content, label, "sans-bold"};
            l->set_tooltip(tooltip);
            auto section = new Widget{ scroll_content };
            section->set_layout(new BoxLayout{ Orientation::Vertical, Alignment::Fill, 0, 0 });
            return section;
        };

        auto add_row = [](Widget* current, string keys, string desc) {
            auto row = new Widget{ current };
            row->set_layout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill, 0, 10 });
            auto desc_widget = new Label{ row, desc};
            desc_widget->set_fixed_width(250);
            new Label{ row, keys, "sans-bold" };
            return desc_widget;
        };

        auto mouse_modes = add_shortcut_section("Mouse modes", "Changes the mouse button mapping, hover on each mode to see what it does.");
        add_row(mouse_modes, "R", "Rotation mode")->set_tooltip("(Left -> Rotate, Middle -> Translate, Right -> Select)");
        add_row(mouse_modes, "T", "Translation mode")->set_tooltip("(Left -> Translate, Middle -> Select, Right -> Rotate)");
        add_row(mouse_modes, "B", "Box Selection mode")->set_tooltip("(Left -> Select, Middle -> Rotate, Right -> Translate)");

        auto move_controls = add_shortcut_section("Move controls");
        add_row(move_controls, "Rotate button drag", "Rotate BSDF visualization");
        add_row(move_controls, "Translate button drag", "Translate BSDF visualization");
        add_row(move_controls, "Mouse wheel", "Zoom in / out");
        add_row(move_controls, "C", "Snap to selection center");

        auto file_loading = add_shortcut_section("File loading");
#if !defined(EMSCRIPTEN)
        add_row(file_loading, COMMAND + "+O", "Open material file");
        add_row(file_loading, COMMAND + "+S", "Save data in the same format");
        add_row(file_loading, COMMAND + "+P", "Save a screenshot");
#endif
        add_row(file_loading, "Delete", "Remove the loaded material");

        auto data_sample_view_options = add_shortcut_section("Data Sample View Options", "For the currently selected material");
        add_row(data_sample_view_options, "L", "Toggle logarithmic view");
        add_row(data_sample_view_options, "P", "Show/hide measurement path");
        add_row(data_sample_view_options, "Shift+P", "Show/hide sample points");
        add_row(data_sample_view_options, "Shift+I", "Show/hide incident angle");


        auto view_options = add_shortcut_section("View Options");
        add_row(view_options, "G", "Show/hide crid");
        add_row(view_options, "A", "Show/hide center axis");
        add_row(view_options, "Shift+G", "Show/hide grid angles");
        add_row(view_options, "Shift+S", "Use/Un-use shadows");
        add_row(view_options, "O or KP_5 or Alt+5", "Enable/Disable orthographic view");
        add_row(view_options, "KP_1 or Alt+1", "Front view");
        add_row(view_options, "KP_3 or Alt+3", "Left view");
        add_row(view_options, "KP_7 or Alt+7", "Top view");
        add_row(view_options, COMMAND + "+KP_1 or " + COMMAND + "+Alt+1", "Back view");
        add_row(view_options, COMMAND + "+KP_3 or " + COMMAND + "+Alt+3", "Right view");
        add_row(view_options, COMMAND + "+KP_7 or " + COMMAND + "+Alt+7", "Bottom view");
        add_row(view_options, "F1", "Show/hide windows (for screenshots)");

        auto data_selection = add_shortcut_section("Loaded material panel");
        add_row(data_selection, "1...9", "Select N-th material");
        add_row(data_selection, "Down or S / Up or W", "Select next / previous material");
        add_row(data_selection, "Left Click", "Select material below mouse cursor");

        auto data_edition = add_shortcut_section("Data selection/editing");
        add_row(data_edition, "Select Button Drag", "Select data points in region");
        add_row(data_edition, "Shift + Select Button Drag", "Add points to current selection");
        add_row(data_edition, "Alt + Select Button Drag", "Remove points from current selection");
        add_row(data_edition, "Select Button Click", "Select closest point (in range)");
        add_row(data_edition, "KP_+ / KP_-", "Move selected points up/down along path");
        add_row(data_edition, "Shift+H / Shift+L", "Select highest/lowest point of selection");
        add_row(data_edition, "Escape", "Deselect all points");

        auto ui = add_shortcut_section("Interface");
        add_row(ui, "H", "Show help (this Window)");
        add_row(ui, "I", "Show metadata");
        add_row(ui, "M", "Chose color map");
#if !defined(EMSCRIPTEN)
        add_row(ui, "Q", "Quit");
#endif
    }


    // About tab
    {
        Widget* about = tab_widget->create_tab("About");
        about->set_layout(new GroupLayout{});

        auto add_text = [](Widget* current, string text, string font = "sans", int font_size = 18) {
            auto row = new Widget{ current };
            row->set_layout(new BoxLayout{ Orientation::Vertical, Alignment::Middle, 0, 10 });
            new Label{ row, text, font, font_size };
        };

        auto add_library = [](Widget* current, string name, string license, string desc) {
            auto row = new Widget{ current };
            row->set_layout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill, 5, 30 });
            auto left_column = new Widget{ row };
            left_column->set_layout(new BoxLayout{ Orientation::Vertical, Alignment::Maximum });
            left_column->set_fixed_width(130);

            new Label{ left_column, name, "sans-bold", 18 };
            new Label{ row, desc, "sans", 18 };
        };

        auto add_spacer = [](Widget* current, int space) {
            auto row = new Widget{ current };
            row->set_height(space);
        };

        add_spacer(about, 15);

        add_text(about, "Tekari", "sans-bold", 46);

        add_spacer(about, 60);

        add_text(about, "This helper tool was developed by Benoit Ruiz and is released");
        add_text(about, "under the BSD 3 - Clause License.");
        add_text(about, "It was built directly or indirectly upon the following amazing third-party libraries.");

        add_spacer(about, 40);

        add_library(about, "Eigen", "", "C++ Template Library for Linear Algebra.");
        add_library(about, "Glad", "", "Multi-Language GL Loader-Generator.");
        add_library(about, "GLEW", "", "The OpenGL Extension Wrangler Library.");
        add_library(about, "GLFW", "", "OpenGL Desktop Development Library.");
        add_library(about, "NanoGUI", "", "Small Widget Library for OpenGL.");
        add_library(about, "NanoVG", "", "Small Vector Graphics Library.");
        add_library(about, "Triangle", "", "C Delaunay Triangulator.");
    }

    tab_widget->set_active_tab(0);
}

bool HelpWindow::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Window::keyboard_event(key, scancode, action, modifiers)) {
        return true;
    }

    if (key == GLFW_KEY_ESCAPE) {
        m_close_callback();
        return true;
    }

    return false;
}

void HelpWindow::perform_layout(NVGcontext* ctx)
{
    Window::perform_layout(ctx);
    //m_scroll_panel->set_fixed_height(m_parent->height() / 2);
    center();
}

TEKARI_NAMESPACE_END
