#include "bsdf_application.h"

using namespace nanogui;

BSDFApplication::BSDFApplication()
:   nanogui::Screen(Eigen::Vector2i(1000, 800), "BSDF Visualizer", false)
,   showNormal(true)
,   showLog(false)
,   showSensorPath(false)
,   showPointHeights(false)
{

    m_VerticalScreenSplit = new Widget{this};
    m_VerticalScreenSplit->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill});

    auto horizontalScreenSplit = new Widget(m_VerticalScreenSplit);
    horizontalScreenSplit->setLayout(new BoxLayout{Orientation::Horizontal, Alignment::Fill});

    m_Sidebar = new Widget(horizontalScreenSplit);
    m_Sidebar->setFixedWidth(200);

    m_HelpButton = new Button(m_Sidebar, "", ENTYPO_ICON_HELP);
    m_HelpButton->setCallback([this]() { std::cout << "Hello " << std::endl; });
    m_HelpButton->setFontSize(15);
    m_HelpButton->setTooltip("Information about using tev.");

    m_SidebarLayout = new Widget(m_Sidebar);
    m_SidebarLayout->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 0));

    // Different view options
    {
        auto panel = new Widget(m_SidebarLayout);
        panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5));
        new Label(panel, "View Options", "sans-bold", 25);
        panel->setTooltip(
                "Various view modes. Hover on them to learn what they do."
            );

        panel = new Widget(m_SidebarLayout);
        panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5, 5));

        auto normalViewToggle = new Button(panel, "Normal");
        normalViewToggle->setFlags(Button::Flags::ToggleButton);
        normalViewToggle->setTooltip("Display/Hide normal view.");
        normalViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->setShowNormalView(checked); });
        normalViewToggle->setPushed(true);

        auto logViewToggle = new Button(panel, "Log");
        logViewToggle->setFlags(Button::Flags::ToggleButton);
        logViewToggle->setTooltip("Display/Hide log view.");
        logViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->setShowLogView(checked); });
        logViewToggle->setPushed(false);

        auto pathViewToggle = new Button(panel, "Path");
        pathViewToggle->setFlags(Button::Flags::ToggleButton);
        pathViewToggle->setTooltip("Display/Hide path view.");
        pathViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->setShowPath(checked); });
        pathViewToggle->setPushed(false);
    }
    // fileName = "path/to/file";
    // imageName = "path/to/image";

    m_BSDFCanvas = new BSDFCanvas(horizontalScreenSplit);
    m_BSDFCanvas->setBackgroundColor({100, 100, 100, 255});

    // FormHelper *options = new FormHelper(this);

    // ref<Window> openFileWindow = options->addWindow(Eigen::Vector2i(10, 10), "Open file");
    // auto fileNameWidget = options->addVariable("File name", fileName);
    // fileNameWidget->setSize({200, 20});
    // fileNameWidget->setFontSize(20);
    // options->addButton("Open", [openFileWindow, this]() mutable {
    //     mCanvas->openFile(fileName);
    //     openFileWindow->setVisible(false);
    // });
    // openFileWindow->center();
    // openFileWindow->setVisible(false);

    // ref<Window> saveImageWindow = options->addWindow(Eigen::Vector2i(0, 0), "Save image");
    // options->addVariable("Image name", imageName);
    // options->addButton("Open", [saveImageWindow, this]() mutable {
    //     std::cout << "Saved image " << fileName << std::endl;
    //     saveImageWindow->setVisible(false);
    // });
    // saveImageWindow->center();
    // saveImageWindow->setVisible(false);

    // ref<Window> optionsWindow = options->addWindow(Eigen::Vector2i(0, 0), "Options");
    // options->addGroup("Additional Info");
    // options->addVariable("Show normal view", showNormal)->setCallback(
    //     [this](bool checked) { mCanvas->setShowNormalView(checked); }
    // );
    // options->addVariable("Show logarithmic view", showLog)->setCallback(
    //     [this](bool checked) { mCanvas->setShowLogView(checked); }
    // );
    // options->addVariable("Show sensor path", showSensorPath)->setCallback(
    //     [this](bool checked) {  mCanvas->setShowPath(checked); }
    // );
    // options->addVariable("Show point heights", showPointHeights);

    // options->addGroup("File");
    // options->addButton("Open", [openFileWindow]() mutable {
    //     openFileWindow->setVisible(true);
    //     openFileWindow->requestFocus();
    // });
    // options->addButton("Save image", [saveImageWindow]() mutable {
    //     saveImageWindow->setVisible(true);
    //     saveImageWindow->requestFocus();
    // });

    setResizeCallback([this](Vector2i) { requestLayoutUpdate(); });

    this->setSize(Vector2i(1024, 800));
}

bool BSDFApplication::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }
    return false;
}

void BSDFApplication::drawContents() {
    if (m_RequiresLayoutUpdate)
    {
        updateLayout();
        m_RequiresLayoutUpdate = false;
    }

}

void BSDFApplication::updateLayout()
{
    m_BSDFCanvas->setFixedSize(mSize - Vector2i{m_Sidebar->width() - 1, -1});
    m_Sidebar->setFixedHeight(mSize.y());

    m_HelpButton->setPosition(Vector2i{m_Sidebar->fixedWidth() - 38, 5});
    m_SidebarLayout->setFixedWidth(m_Sidebar->fixedWidth());

    m_VerticalScreenSplit->setFixedSize(mSize);

    performLayout();

    // With a changed layout the relative position of the mouse
    // within children changes and therefore should get updated.
    // nanogui does not handle this for us.
    double x, y;
    glfwGetCursorPos(mGLFWWindow, &x, &y);
    cursorPosCallbackEvent(x, y);
}