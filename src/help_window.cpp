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

        auto mouse_modes = add_shortcut_section("Mouse_modes", "Changes the mouse buttons mapping, hover on each mode to see what it does.");
        add_row(mouse_modes, "R", "Rotation Mode")->set_tooltip("(Left -> Rotate, Middle -> Translate, Right -> Select)");
        add_row(mouse_modes, "T", "Translation Mode")->set_tooltip("(Left -> Translate, Middle -> Select, Right -> Rotate)");
        add_row(mouse_modes, "B", "Box Selection Mode")->set_tooltip("(Left -> Select, Middle -> Rotate, Right -> Translate)");

        auto move_controls = add_shortcut_section("Move Controls");
        add_row(move_controls, "Rotate Button Drag", "Rotate Data Sample");
        add_row(move_controls, "Translate Button Drag", "Translate Data Sample");
        add_row(move_controls, "Scroll In / Out", "Zoom In / Out");
        add_row(move_controls, "C", "Snap To Selection Center");

        auto file_loading = add_shortcut_section("File Loading");
        add_row(file_loading, COMMAND + "+O", "Open Data Samples");
        add_row(file_loading, COMMAND + "+S", "Save Data in the same format");
        add_row(file_loading, COMMAND + "+P", "Save Screenshot of Data");
        add_row(file_loading, "Delete", "Close Selected Data Sample");

        auto data_sample_view_options = add_shortcut_section("Data Sample View Options", "For the currently selected datasample");
        add_row(data_sample_view_options, "L", "Toggle Logarithmic View");
        add_row(data_sample_view_options, "P", "Show/Hide Path");
        add_row(data_sample_view_options, "Shift+P", "Show/Hide All Points");
        add_row(data_sample_view_options, "Shift+I", "Show/Hide Incident Angle");


        auto view_options = add_shortcut_section("View Options");
        add_row(view_options, "G", "Show/Hide Grid");
        add_row(view_options, "A", "Show/Hide Center Axis");
        add_row(view_options, "Shift+G", "Show/Hide Grid Degrees");
        add_row(view_options, "Shift+S", "Use/Un-use Shadows");
        add_row(view_options, "O or KP_5 or Alt+5", "Enable/Disable Orthographic View");
        add_row(view_options, "KP_1 or Alt+1", "Front View");
        add_row(view_options, "KP_3 or Alt+3", "Left View");
        add_row(view_options, "KP_7 or Alt+7", "Top View");
        add_row(view_options, COMMAND + "+KP_1 or " + COMMAND + "+Alt+1", "Back View");
        add_row(view_options, COMMAND + "+KP_3 or " + COMMAND + "+Alt+3", "Right View");
        add_row(view_options, COMMAND + "+KP_7 or " + COMMAND + "+Alt+7", "Bottom View");
        add_row(view_options, "F1", "Show/Hide all the windows (to make screenshots cleaner)");

        auto data_selection = add_shortcut_section("Data Sample Selection");
        add_row(data_selection, "1...9", "Select N-th Data Sample");
        add_row(data_selection, "Down or S / Up or W", "Select Next / Previous Data Sample");
        add_row(data_selection, "Left Click", "Select Hovered Data Sample");

        auto data_edition = add_shortcut_section("Data Selection/Editing");
        add_row(data_edition, "Select Button Drag", "Select Data Points In Region");
        add_row(data_edition, "Shift + Select Button Drag", "Add Points To Current Selection");
        add_row(data_edition, "Alt + Select Button Drag", "Remove Points To Current Selection");
        add_row(data_edition, "Select Button Click", "Select Closest Point (In Range)");
        add_row(data_edition, "KP_+ / KP_-", "Move Selected Points Up/Down Allong Path");
        add_row(data_edition, "Shift+H / Shift+L", "Select highest/lowest point of selection");
        add_row(data_edition, "Escape", "Deselect All Points");

        auto ui = add_shortcut_section("Interface");
        add_row(ui, "H", "Show Help (this Window)");
        add_row(ui, "I", "Show Metadata");
        add_row(ui, "M", "Chose Color Map");
        add_row(ui, "Q", "Quit");
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