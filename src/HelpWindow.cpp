#include "tekari/HelpWindow.h"

#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/tabwidget.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/window.h>

using namespace nanogui;
using namespace std;

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


HelpWindow::HelpWindow(Widget *parent, function<void()> closeCallback)
:   Window{ parent, "Help" }
,   mCloseCallback{ closeCallback }
{
    setFixedWidth(600);
    auto closeButton = new Button{ buttonPanel(), "", ENTYPO_ICON_CROSS };
    closeButton->setCallback(mCloseCallback);
    setLayout(new GroupLayout{});

    TabWidget* tabWidget = new TabWidget{ this };

    // Keybindings tab
    {
        Widget* shortcuts = tabWidget->createTab("Keybindings");
        shortcuts->setLayout(new GroupLayout{});

        mScrollPanel = new VScrollPanel{ shortcuts };
        auto scrollContent = new Widget{ mScrollPanel };
        scrollContent->setLayout(new GroupLayout{});
        mScrollPanel->setFixedHeight(500);

        auto addShortcutSection = [&scrollContent](const std::string& label, const std::string& tooltip="") {
            auto l = new Label{ scrollContent, label, "sans-bold", 18 };
            l->setTooltip(tooltip);
            auto section = new Widget{ scrollContent };
            section->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Fill, 0, 0 });
            return section;
        };

        auto addRow = [](Widget* current, string keys, string desc) {
            auto row = new Widget{ current };
            row->setLayout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill, 0, 10 });
            auto descWidget = new Label{ row, desc, "sans" };
            descWidget->setFixedWidth(250);
            new Label{ row, keys, "sans-bold" };
            return descWidget;
        };

        auto mouseModes = addShortcutSection("MouseModes", "Changes the mouse buttons mapping, hover on each mode to see what it does.");
        addRow(mouseModes, "R", "Rotation Mode")->setTooltip("(Left -> Rotate, Middle -> Translate, Right -> Select)");
        addRow(mouseModes, "T", "Translation Mode")->setTooltip("(Left -> Translate, Middle -> Select, Right -> Rotate)");
        addRow(mouseModes, "B", "Box Selection Mode")->setTooltip("(Left -> Select, Middle -> Rotate, Right -> Translate)");

        auto moveControls = addShortcutSection("Move Controls");
        addRow(moveControls, "Rotate Button Drag", "Rotate Data Sample");
        addRow(moveControls, "Translate Button Drag", "Translate Data Sample");
        addRow(moveControls, "Scroll In / Out", "Zoom In / Out");
        addRow(moveControls, "C", "Snap To Selection Center");

        auto fileLoading = addShortcutSection("File Loading");
        addRow(fileLoading, COMMAND + "+O", "Open Data Sample");
        addRow(fileLoading, COMMAND + "+S", "Save Data in obj format");
        addRow(fileLoading, COMMAND + "+P", "Save Screenshot of Data");
        addRow(fileLoading, "Delete", "Close Selected Data Sample");

        auto dataSampleViewOptions = addShortcutSection("Data Sample View Options", "For the currently selected datasample");
        addRow(dataSampleViewOptions, "N", "Switch To Normal View");
        addRow(dataSampleViewOptions, "L", "Switch To Logarithmic View");
        addRow(dataSampleViewOptions, "P", "Show/Hide Path");
        addRow(dataSampleViewOptions, "Shift+P", "Show/Hide All Points");
        addRow(dataSampleViewOptions, "Shift+I", "Show/Hide Incident Angle");


        auto viewOptions = addShortcutSection("View Options");
        addRow(viewOptions, "G", "Show/Hide Grid");
        addRow(viewOptions, "A", "Show/Hide Center Axis");
        addRow(viewOptions, "Shift+G", "Show/Hide Grid Degrees");
        addRow(viewOptions, "Shift+S", "Use/Un-use Shadows");
        addRow(viewOptions, "O or KP_5 or Alt+5", "Enable/Disable Orthographic View");
        addRow(viewOptions, "KP_1 or Alt+1", "Front View");
        addRow(viewOptions, "KP_3 or Alt+3", "Left View");
        addRow(viewOptions, "KP_7 or Alt+7", "Top View");
        addRow(viewOptions, COMMAND + "+KP_1 or " + COMMAND + "+Alt+1", "Back View");
        addRow(viewOptions, COMMAND + "+KP_3 or " + COMMAND + "+Alt+3", "Right View");
        addRow(viewOptions, COMMAND + "+KP_7 or " + COMMAND + "+Alt+7", "Bottom View");

        auto dataSelection = addShortcutSection("Data Sample Selection");
        addRow(dataSelection, "1...9", "Select N-th Data Sample");
        addRow(dataSelection, "Down or S / Up or W", "Select Next / Previous Data Sample");
        addRow(dataSelection, "Left Click", "Select Hovered Data Sample");

        auto dataEdition = addShortcutSection("Data Selection/Editing");
        addRow(dataEdition, "Select Button Drag", "Select Data Points In Region");
        addRow(dataEdition, "Shift + Select Button Drag", "Add Points To Current Selection");
        addRow(dataEdition, "Alt + Select Button Drag", "Remove Points To Current Selection");
        addRow(dataEdition, "Select Button Click", "Select Closest Point (In Range)");
        addRow(dataEdition, "KP_+ / KP_-", "Move Selected Points Allong Path");
        addRow(dataEdition, "Escape", "Deselect All Points");

        auto ui = addShortcutSection("Interface");
        addRow(ui, "H", "Show Help (this Window)");
        addRow(ui, "I", "Show Metadata");
        addRow(ui, "M", "Chose Color Map");
        addRow(ui, "Q", "Quit");
    }


    // About tab
    {
        Widget* about = tabWidget->createTab("About");
        about->setLayout(new GroupLayout{});

        auto addText = [](Widget* current, string text, string font = "sans", int fontSize = 18) {
            auto row = new Widget{ current };
            row->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Middle, 0, 10 });
            new Label{ row, text, font, fontSize };
        };

        auto addLibrary = [](Widget* current, string name, string license, string desc) {
            auto row = new Widget{ current };
            row->setLayout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill, 5, 30 });
            auto leftColumn = new Widget{ row };
            leftColumn->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Maximum });
            leftColumn->setFixedWidth(130);

            new Label{ leftColumn, name, "sans-bold", 18 };
            new Label{ row, desc, "sans", 18 };
        };

        auto addSpacer = [](Widget* current, int space) {
            auto row = new Widget{ current };
            row->setHeight(space);
        };

        addSpacer(about, 15);

        addText(about, "Tekari", "sans-bold", 46);

        addSpacer(about, 60);

        addText(about, "This helper tool was developed by Benoit Ruiz and is released");
        addText(about, "under the BSD 3 - Clause License.");
        addText(about, "It was built directly or indirectly upon the following amazing third-party libraries.");

        addSpacer(about, 40);

        addLibrary(about, "Eigen", "", "C++ Template Library for Linear Algebra.");
        addLibrary(about, "Glad", "", "Multi-Language GL Loader-Generator.");
        addLibrary(about, "GLEW", "", "The OpenGL Extension Wrangler Library.");
        addLibrary(about, "GLFW", "", "OpenGL Desktop Development Library.");
        addLibrary(about, "NanoGUI", "", "Small Widget Library for OpenGL.");
        addLibrary(about, "NanoVG", "", "Small Vector Graphics Library.");
    }

    tabWidget->setActiveTab(0);
}

bool HelpWindow::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Window::keyboardEvent(key, scancode, action, modifiers)) {
        return true;
    }

    if (key == GLFW_KEY_ESCAPE) {
        mCloseCallback();
        return true;
    }

    return false;
}

void HelpWindow::performLayout(NVGcontext *ctx)
{
    Window::performLayout(ctx);
    //mScrollPanel->setFixedHeight(mParent->height() / 2);
    center();
}

TEKARI_NAMESPACE_END