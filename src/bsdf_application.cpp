#include "bsdf_application.h"

using namespace nanogui;

BSDFApplication::BSDFApplication()
:   nanogui::Screen(Eigen::Vector2i(1000, 800), "BSDF Visualizer", true)
,   showNormal(true)
,   showLog(false)
,   showSensorPath(false)
,   showPointHeights(false)
{
	m_BSDFCanvas = new BSDFCanvas(this);
	m_BSDFCanvas->setBackgroundColor({ 50, 50, 50, 255 });

	m_ToolWindow = new Window(this, "Tools");
	m_ToolWindow->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 0, 0});
	m_ToolWindow->setVisible(true);

    m_HelpButton = new Button(m_ToolWindow->buttonPanel(), "", ENTYPO_ICON_HELP);
    m_HelpButton->setCallback([this]() { std::cout << "Help button triggered." << std::endl; });
    m_HelpButton->setFontSize(15);
    m_HelpButton->setTooltip("Information about using BSDFV.");
	m_HelpButton->setPosition({20, 0});

    // Different view options
    {
        auto panel = new Widget(m_ToolWindow);
        panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5));
        new Label(panel, "View Options", "sans-bold", 25);
        panel->setTooltip(
                "Various view modes. Hover on them to learn what they do."
            );

        // sample data view options
        panel = new Widget(m_ToolWindow);
        panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5, 5));

        auto normalViewToggle = new Button(panel, "Normal");
        normalViewToggle->setFlags(Button::Flags::ToggleButton);
        normalViewToggle->setTooltip("Display/Hide normal view.");
        normalViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->sampleData().displayNormalView(checked); });
        normalViewToggle->setPushed(true);

        auto logViewToggle = new Button(panel, "Log");
        logViewToggle->setFlags(Button::Flags::ToggleButton);
        logViewToggle->setTooltip("Display/Hide log view.");
        logViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->sampleData().displayLogView(checked); });
        logViewToggle->setPushed(false);

        auto pathViewToggle = new Button(panel, "Path");
        pathViewToggle->setFlags(Button::Flags::ToggleButton);
        pathViewToggle->setTooltip("Display/Hide path view.");
        pathViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->sampleData().displayPath(checked); });
        pathViewToggle->setPushed(false);

        // grid view otpions
        panel = new Widget(m_ToolWindow);
        panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5, 5));

        auto gridViewToggle = new Button(panel, "Grid");
        gridViewToggle->setFlags(Button::Flags::ToggleButton);
        gridViewToggle->setTooltip("Display/Hide grid.");
        gridViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->grid().setVisible(checked); });
        gridViewToggle->setPushed(true);

		auto orthoViewToggle = new Button(panel, "Ortho");
		orthoViewToggle->setFlags(Button::Flags::ToggleButton);
		orthoViewToggle->setTooltip("Enable/Disable orthogonal projection.");
		orthoViewToggle->setChangeCallback([this](bool checked) { m_BSDFCanvas->setOrthoMode(checked); });
		orthoViewToggle->setPushed(false);

// TODO: this doesn't work for some reason

        auto gridColorPopupButton = new PopupButton(panel, "", ENTYPO_ICON_BUCKET);

        gridColorPopupButton->setFontSize(15);
        gridColorPopupButton->setChevronIcon(0);
        gridColorPopupButton->setTooltip("Grid Color");

        // Background color popup
        {
            auto popup = gridColorPopupButton->popup();
            popup->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 10});

            new Label{popup, "Grid Color"};
            auto colorwheel = new ColorWheel{popup, m_BSDFCanvas->grid().color()};

            new Label{popup, "Grid Alpha"};
            auto gridAlphaSlider = new Slider{popup};
            gridAlphaSlider->setRange({0.0f, 1.0f});
            gridAlphaSlider->setCallback([this](float value) {
                auto col = m_BSDFCanvas->grid().color();
                m_BSDFCanvas->grid().setColor(Color{
                    col.r(),
                    col.g(),
                    col.b(),
                    value,
                });
            });

            gridAlphaSlider->setValue(1.0);

            colorwheel->setCallback([gridAlphaSlider, this](const Color& value) {
                //popupBtn->setBackgroundColor(value);
                m_BSDFCanvas->grid().setColor(Color{
                    value.r(),
                    value.g(),
                    value.b(),
                    gridAlphaSlider->value(),
                });
            });
        }

    }

    // Save, refresh, load, close
    {
        new Label(m_ToolWindow, "File", "sans-bold", 25);
        auto tools = new Widget{ m_ToolWindow };
        tools->setLayout(new GridLayout{Orientation::Horizontal, 5, Alignment::Fill, 5, 1});

        auto makeImageButton = [&](const std::string& name, bool enabled, std::function<void()> callback, int icon = 0, std::string tooltip = "") {
            auto button = new Button{tools, name, icon};
            button->setCallback(callback);
            button->setTooltip(tooltip);
            button->setFontSize(15);
            button->setEnabled(enabled);
            return button;
        };

        makeImageButton("", true, [this] {
            openDataSampleDialog();
        }, ENTYPO_ICON_FOLDER, "Open data sample (CTRL+O)");

        makeImageButton("", true, [this] {
            saveScreenShot();
        }, ENTYPO_ICON_SAVE, "Save image (CTRL+S)");

        makeImageButton("", true, [this] {
            closeSelectedDataSample();
        }, ENTYPO_ICON_CROSS, "Close (CTRL+W)");

        auto spacer = new Widget{ m_ToolWindow };
        spacer->setHeight(3);
    }

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
    m_BSDFCanvas->setFixedSize(mSize);
	m_ToolWindow->setFixedSize({ 200, 400 });
	m_ToolWindow->setPosition({ 0, 0 });

    performLayout();

    // With a changed layout the relative position of the mouse
    // within children changes and therefore should get updated.
    // nanogui does not handle this for us.
    double x, y;
    glfwGetCursorPos(mGLFWWindow, &x, &y);
    cursorPosCallbackEvent(x, y);
}

void BSDFApplication::openDataSampleDialog()
{
    std::string dataSamplePath = file_dialog(
    {
        {"txt",  "Data samples"},
    }, false);
    
	if (!dataSamplePath.empty())
	{
		m_BSDFCanvas->openFile(dataSamplePath);
	}
    // Make sure we gain focus after seleting a file to be loaded.
    glfwFocusWindow(mGLFWWindow);
}

void BSDFApplication::saveScreenShot()
{
    std::cout << "Save screenshot." << std::endl;

    // TODO ask how this is suppose to work
    nanogui::GLFramebuffer offscreenBuffer;
    offscreenBuffer.init(m_BSDFCanvas->size(), 1);
    offscreenBuffer.bind();

    m_BSDFCanvas->drawGL();

    offscreenBuffer.downloadTGA("test.tga");
    offscreenBuffer.free();

}

void BSDFApplication::closeSelectedDataSample()
{
    std::cout << "Close current data sample." << std::endl;
}