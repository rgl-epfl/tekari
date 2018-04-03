#include "BSDFApplication.h"

#include <nanogui\layout.h>
#include <nanogui\button.h>
#include <nanogui\entypo.h>
#include <nanogui\popupbutton.h>
#include <nanogui\colorwheel.h>
#include <nanogui\slider.h>
#include <nanogui\vscrollpanel.h>
#include <nanogui\messagedialog.h>
#include <nanogui\label.h>

#include <algorithm>
#include <string>

using namespace nanogui;

BSDFApplication::BSDFApplication()
:   nanogui::Screen(Eigen::Vector2i(1200, 750), "BSDF Visualizer", true)
,	m_SelectedDataSampleIndex(-1)
,	m_MetadataWindow(nullptr)
,	m_HelpWindow(nullptr)
{
    m_BSDFCanvas = new BSDFCanvas(this);
    m_BSDFCanvas->setBackgroundColor({ 50, 50, 50, 255 });

    m_ToolWindow = new Window(this, "Tools");
    m_ToolWindow->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 5, 5});
    m_ToolWindow->setVisible(true);

    m_HelpButton = new Button(m_ToolWindow->buttonPanel(), "", ENTYPO_ICON_HELP);
    m_HelpButton->setCallback([this]() { toggleHelpWindow(); });
    m_HelpButton->setFontSize(15);
    m_HelpButton->setTooltip("Information about using BSDF Vidualizer (H)");
    m_HelpButton->setPosition({20, 0});

    // Different view options for the selected data sample
    {
        auto label = new Label(m_ToolWindow, "View Options", "sans-bold", 25);
        label->setTooltip(
            "Various view modes. Hover on them to learn what they do."
        );

        // sample data view options
        m_ViewButtonsContainer = new Widget(m_ToolWindow);
        m_ViewButtonsContainer->setLayout(new GridLayout(Orientation::Horizontal, 3, Alignment::Fill));

        auto makeViewButton = [this](const std::string& label, const std::string& tooltip,
            std::function<void(bool)> changeCallback)
        {
            auto button = new Button(m_ViewButtonsContainer, label);
            button->setFlags(Button::Flags::ToggleButton);
            button->setTooltip(tooltip);
            button->setChangeCallback(changeCallback);
        };

        makeViewButton("Normal", "Display/Hide normal view (N)", [this](bool) { toggleView(DataSample::NORMAL); });
        makeViewButton("Log", "Display/Hide logarithmic view (L)", [this](bool) { toggleView(DataSample::LOG); });
        makeViewButton("Path", "Display/Hide path (P)", [this](bool) { toggleView(DataSample::PATH); });

    }
    // grid view otpions
    {
        auto panel = new Widget(m_ToolWindow);
        panel->setLayout(new GridLayout(Orientation::Horizontal, 3, Alignment::Fill));

        m_GridViewToggle = new Button(panel, "Grid");
        m_GridViewToggle->setFlags(Button::Flags::ToggleButton);
        m_GridViewToggle->setTooltip("Display/Hide grid (G)");
        m_GridViewToggle->setChangeCallback( [this] (bool checked) { m_BSDFCanvas->grid().setVisible(checked); });
        m_GridViewToggle->setPushed(true);

        m_OrthoViewToggle = new Button(panel, "Ortho");
        m_OrthoViewToggle->setFlags(Button::Flags::ToggleButton);
        m_OrthoViewToggle->setTooltip("Enable/Disable orthogonal projection (O)");
        m_OrthoViewToggle->setChangeCallback([this](bool checked) { m_BSDFCanvas->setOrthoMode(checked); });
        m_OrthoViewToggle->setPushed(false);

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

    // Open, save screenshot, save data
    {
        new Label(m_ToolWindow, "Data Samples", "sans-bold", 25);
        auto tools = new Widget{ m_ToolWindow };
        tools->setLayout(new GridLayout{Orientation::Horizontal, 4, Alignment::Fill});

        auto makeToolButton = [&](bool enabled, std::function<void()> callback, int icon = 0, std::string tooltip = "") {
            auto button = new Button{tools, "", icon};
            button->setCallback(callback);
            button->setTooltip(tooltip);
            button->setFontSize(15);
            button->setEnabled(enabled);
            return button;
        };

        makeToolButton(true, [this] { openDataSampleDialog(); }, ENTYPO_ICON_FOLDER, "Open data sample (CTRL+O)");
        makeToolButton(true, [this] { saveScreenShot(); }, ENTYPO_ICON_IMAGE, "Save image (CTRL+P)");
        makeToolButton(true, [this] { std::cout << "Data saved\n"; }, ENTYPO_ICON_SAVE, "Save data (CTRL+S)");
        makeToolButton(true, [this]() { toggleMetadataWindow(); }, ENTYPO_ICON_INFO, "Show selected dataset infos (I)");
    }

    // Data sample selection
    {
        m_DataSamplesScrollPanel = new VScrollPanel{ m_ToolWindow };

        m_ScrollContent = new Widget{ m_DataSamplesScrollPanel };
        m_ScrollContent->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill));

        m_DataSampleButtonContainer = new Widget{ m_ScrollContent };
        m_DataSampleButtonContainer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 0));
    }

    refreshToolButtons();

    m_ColorMaps.push_back(std::make_shared<ColorMap>("../resources/color_maps/inferno.png"));
    m_ColorMaps.push_back(std::make_shared<ColorMap>("../resources/color_maps/jet.png"));

    setResizeCallback([this](Vector2i) { requestLayoutUpdate(); });
}

BSDFApplication::~BSDFApplication()
{
    m_Framebuffer.free();
}

bool BSDFApplication::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers))
        return true;
    if (action == GLFW_PRESS)
    {
        // control options
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            switch (key)
            {
            case GLFW_KEY_O:
                openDataSampleDialog();
                break;
            case GLFW_KEY_S:
                std::cout << "Save data\n";
                break;
            case GLFW_KEY_P:
                saveScreenShot();
                break;
            }
        }
        else
        {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                setVisible(false);
                return true;
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4: case GLFW_KEY_5:
            case GLFW_KEY_6: case GLFW_KEY_7: case GLFW_KEY_8: case GLFW_KEY_9:
                selectDataSample(key - GLFW_KEY_1);
                return true;
            case GLFW_KEY_DELETE:
                if (hasSelectedDataSample())
                {
                    auto buttonToDelete = m_DataSampleButtonContainer->childAt(m_SelectedDataSampleIndex);
                    deleteDataSample(dynamic_cast<DataSampleButton*>(buttonToDelete));
                    return true;
                }
                break;
            case GLFW_KEY_UP:
            case GLFW_KEY_DOWN:
                selectDataSample(m_SelectedDataSampleIndex + (key == GLFW_KEY_UP ? -1 : 1), false);
                return true;
                break;
            case GLFW_KEY_ENTER:
                if (hasSelectedDataSample())
                {
                    auto w = m_DataSampleButtonContainer->childAt(m_SelectedDataSampleIndex);
                    DataSampleButton* button = dynamic_cast<DataSampleButton*>(w);
                    button->toggleView();
                    return true;
                }
                break;
            case GLFW_KEY_N:
                toggleView(DataSample::Views::NORMAL);
                return true;
            case GLFW_KEY_L:
                toggleView(DataSample::Views::LOG);
                return true;
            case GLFW_KEY_P:
                toggleView(DataSample::Views::PATH);
                return true;
            case GLFW_KEY_G:
                toggleToolButton(m_GridViewToggle, false);
                return true;
            case GLFW_KEY_O:
                toggleToolButton(m_OrthoViewToggle, false);
                return true;
            case GLFW_KEY_I:
                toggleMetadataWindow();
                return true;
            case GLFW_KEY_H:
                toggleHelpWindow();
                return true;
            default:
                break;
            }
        }
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

    m_DataSamplesScrollPanel->setFixedHeight(
        m_ToolWindow->height() - m_DataSamplesScrollPanel->position().y()
    );

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
        try
        {
            std::shared_ptr<DataSample> newDataSample = std::make_shared<DataSample>(m_ColorMaps[1], dataSamplePath);
            m_DataSamples.push_back(newDataSample);
            addDataSampleButton(m_DataSamples.size() - 1, newDataSample);
        }
        catch (std::exception e)
        {
            auto errorMsgDialog = new MessageDialog(this, MessageDialog::Type::Warning, "Error loading data",
                e.what(), "Retry", "Cancel", true);
            errorMsgDialog->setCallback([this](int index)
            {
                if (index == 0) { openDataSampleDialog(); }
            });
        }
    }
    // Make sure we gain focus after seleting a file to be loaded.
    glfwFocusWindow(mGLFWWindow);
}

void BSDFApplication::saveScreenShot()
{
    if (!m_Framebuffer.ready())
    {
        m_Framebuffer.init(m_BSDFCanvas->size(), 1);
    }
    m_Framebuffer.bind();
    m_BSDFCanvas->draw(nvgContext());
    m_Framebuffer.downloadTGA("test.tga");
    m_Framebuffer.release();
}

void BSDFApplication::toggleMetadataWindow()
{
    if (m_MetadataWindow)
    {
        m_MetadataWindow->dispose();
        m_MetadataWindow = nullptr;
    }
    else
    {
        if (hasSelectedDataSample())
        {
            m_MetadataWindow = new MetadataWindow(this, &getSelectedDataSample()->metadata(), [this]() { toggleMetadataWindow(); });
            m_MetadataWindow->center();
            m_MetadataWindow->requestFocus();
        }
        else
        {
            auto errorWindow = new MessageDialog(this, MessageDialog::Type::Warning, "Metadata",
                "No data sample selected.", "close");
            errorWindow->setCallback([this](int index) { m_MetadataWindow = nullptr; });
            m_MetadataWindow = errorWindow;
        }
    }
}

void BSDFApplication::toggleHelpWindow()
{
    if (m_HelpWindow)
    {
        m_HelpWindow->dispose();
        m_HelpWindow = nullptr;
    }
    else
    {
        m_HelpWindow = new HelpWindow(this, [this]() {toggleHelpWindow(); });
        m_HelpWindow->center();
        m_HelpWindow->requestFocus();
    }
}

void BSDFApplication::selectDataSample(int index, bool clamped)
{
    if (m_DataSamples.empty())
        return;

    if (clamped)
        index = std::max(0, std::min(static_cast<int>(m_DataSamples.size()-1), index));
    else if (index < 0 || index >= m_DataSamples.size())
        return;

    auto buttonToSelect = m_DataSampleButtonContainer->childAt(index);
    selectDataSample(dynamic_cast<DataSampleButton*>(buttonToSelect));
}

void BSDFApplication::selectDataSample(DataSampleButton* button)
{
    // de-select previously selected button
    Widget* oldButton = nullptr;
    if (hasSelectedDataSample())
    {
        oldButton = m_DataSampleButtonContainer->childAt(m_SelectedDataSampleIndex);
        dynamic_cast<DataSampleButton*>(oldButton)->setIsSelected(false);
    }

    if (oldButton == button)
    {
        m_SelectedDataSampleIndex = -1;
    }
    else
    {
        // get button index
        m_SelectedDataSampleIndex = m_DataSampleButtonContainer->childIndex(button);
        // if the button gave a valid index
        if (hasSelectedDataSample())
        {
            // select button
            button->setIsSelected(true);
        }
    }

    refreshToolButtons();
    requestLayoutUpdate();
}

void BSDFApplication::deleteDataSample(DataSampleButton* button)
{
    int index = m_DataSampleButtonContainer->childIndex(button);
    if (index < 0 && index >= m_DataSamples.size())
        return;

    // erase data sample and corresponding button
    m_BSDFCanvas->removeDataSample(m_DataSamples[index]);
    m_DataSamples.erase(m_DataSamples.begin() + index);
    m_DataSampleButtonContainer->removeChild(button);
    // clear focus path and drag widget pointer, since it may refer to deleted button
    mDragWidget = nullptr;
    mDragActive = false;
    mFocusPath.clear();

    // select next valid one
    DataSampleButton* buttonToSelect = nullptr;
    if (index >= m_DataSamples.size()) --index;
    if (index >= 0)
    {
        buttonToSelect = dynamic_cast<DataSampleButton*>(m_DataSampleButtonContainer->childAt(index));
    }
    // Make sure no button is selected
    m_SelectedDataSampleIndex = -1;
    selectDataSample(buttonToSelect);
}

void BSDFApplication::addDataSampleButton(int index, std::shared_ptr<DataSample> dataSample)
{
    std::string cleanName = dataSample->metadata().sampleName;
    std::replace(cleanName.begin(), cleanName.end(), '_', ' ');
    auto dataSampleButton = new DataSampleButton(m_DataSampleButtonContainer, cleanName);
    dataSampleButton->setFixedHeight(30);

    dataSampleButton->setCallback([this](DataSampleButton* w) { selectDataSample(w); });
    dataSampleButton->setDeleteCallback([this](DataSampleButton* w) { deleteDataSample(w); });
    dataSampleButton->setToggleViewCallback([this](bool checked, DataSampleButton* w) {
        int index = m_DataSampleButtonContainer->childIndex(w);
        if (checked)
            m_BSDFCanvas->addDataSample(m_DataSamples[index]);
        else
            m_BSDFCanvas->removeDataSample(m_DataSamples[index]);
    });
    selectDataSample(dataSampleButton);
    // by default toggle view for the new data samples
    m_BSDFCanvas->addDataSample(getSelectedDataSample());
}

void BSDFApplication::refreshToolButtons()
{
    auto& children = m_ViewButtonsContainer->children();
    for (int i = DataSample::Views::NORMAL; i != DataSample::Views::VIEW_COUNT; ++i)
    {
        auto viewButton = dynamic_cast<Button*>(children[i]);
        viewButton->setPushed(hasSelectedDataSample() ? getSelectedDataSample()->displayView((DataSample::Views)i) : false);
        viewButton->setEnabled(hasSelectedDataSample());
    }
}

void BSDFApplication::toggleToolButton(nanogui::Button* button, bool needsSelectedDataSample)
{
    if (!needsSelectedDataSample || hasSelectedDataSample())
    {
        button->setPushed(!button->pushed());
        button->changeCallback()(button->pushed());
    }
}

void BSDFApplication::toggleView(DataSample::Views view)
{
    if (hasSelectedDataSample())
    {
        getSelectedDataSample()->toggleView(view);
        auto button = dynamic_cast<Button*>(m_ViewButtonsContainer->childAt(view));
        button->setPushed(getSelectedDataSample()->displayView(view));
    }
}