#include "tekari/BSDFApplication.h"

#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/popupbutton.h>
#include <nanogui/colorwheel.h>
#include <nanogui/checkbox.h>
#include <nanogui/imageview.h>
#include <nanogui/slider.h>
#include <nanogui/combobox.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/messagedialog.h>
#include <nanogui/label.h>

#include <algorithm>
#include <bitset>
#include <string>

#include "tekari/SelectionBox.h"

using namespace nanogui;
using namespace std;

TEKARI_NAMESPACE_BEGIN

BSDFApplication::BSDFApplication(const std::vector<std::string>& dataSamplePaths)
:   nanogui::Screen(Vector2i(1200, 750), "Tekari", true)
,   m_SelectedDataSample(nullptr)
,	m_MetadataWindow(nullptr)
,   m_HelpWindow(nullptr)
,   m_SelectionInfoWindow(nullptr)
,	m_ColorMapSelectionWindow(nullptr)
{
    // load color maps
    for (auto& p : ColorMap::PREDEFINED_MAPS)
    {
        m_ColorMaps.push_back(make_shared<ColorMap>(p.first, ColorMap::FOLDER_PATH + p.second));
    }

    m_3DView = new Widget{this};
    m_3DView->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Fill });

    // canvas
    m_BSDFCanvas = new BSDFCanvas{ m_3DView };
    m_BSDFCanvas->setBackgroundColor({ 50, 50, 50, 255 });
    m_BSDFCanvas->setSelectionCallback([this](const Matrix4f& mvp, const SelectionBox& selectionBox,
        const Vector2i& canvasSize, SelectionMode mode) {
        if (m_SelectionInfoWindow)
        {
            toggleSelectionInfoWindow();
        }

        for (auto& dataSample : m_DataSamples)
        {
            if (dataSample != m_SelectedDataSample)
            {
                deselect_all_points(dataSample->selectedPoints());
                dataSample->updatePointSelection();
            }
        }
        if (m_SelectedDataSample)
        {
            if (selectionBox.empty())
            {
                select_closest_point(   m_SelectedDataSample->rawPoints(),
                                        m_SelectedDataSample->V2D(),
                                        m_SelectedDataSample->H(),
                                        m_SelectedDataSample->selectedPoints(),
                                        mvp, selectionBox.topLeft, canvasSize);
                m_SelectedDataSample->updatePointSelection();
                toggleSelectionInfoWindow();
            }
            else
            {
                select_points(  m_SelectedDataSample->rawPoints(),
                                m_SelectedDataSample->V2D(),
                                m_SelectedDataSample->H(),
                                m_SelectedDataSample->selectedPoints(),
                                mvp, selectionBox, canvasSize, mode);
                m_SelectedDataSample->updatePointSelection();
                toggleSelectionInfoWindow();
            }
        }
    });
    m_BSDFCanvas->setColorMap(m_ColorMaps[0]);

    // Footer
    {
        m_Footer = new Widget{ m_3DView };
        m_Footer->setLayout(new GridLayout{ Orientation::Horizontal, 3, Alignment::Fill});
    
        auto makeFooterInfo = [this](string label) {
            auto container = new Widget{ m_Footer };
            container->setLayout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill });
            container->setFixedWidth(width() / 3);
            new Label{ container, label };
            auto info = new Label{ container, "-" };
            return info;
        };

        m_DataSampleName = makeFooterInfo("Data Sample Name : ");
        m_DataSamplePointsCount = makeFooterInfo("Points Count : ");
        m_DataSampleAverageHeight = makeFooterInfo("AverageIntensity : ");
    }

    m_ToolWindow = new Window(this, "Tools");
    m_ToolWindow->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 5, 5});
    m_ToolWindow->setVisible(true);
    m_ToolWindow->setPosition({ 20, 20 });

    m_HelpButton = new Button(m_ToolWindow->buttonPanel(), "", ENTYPO_ICON_HELP);
    m_HelpButton->setCallback([this]() { toggleHelpWindow(); });
    m_HelpButton->setFontSize(15);
    m_HelpButton->setTooltip("Information about using BSDF Vidualizer (H)");
    m_HelpButton->setPosition({20, 0});

    // Hidden options
    {
        m_HiddenOptionsButton = new PopupButton(m_ToolWindow->buttonPanel(), "", ENTYPO_ICON_TOOLS);
        m_HiddenOptionsButton->setBackgroundColor(Color{0.4f, 0.1f, 0.1f, 1.0f});
        m_HiddenOptionsButton->setTooltip("More view options");
        auto hiddenOptionsPopup = m_HiddenOptionsButton->popup();
        hiddenOptionsPopup->setLayout(new GroupLayout{});

        auto addHiddenOptionToggle = [hiddenOptionsPopup](const string& label, const string& tooltip,
            const function<void(bool)> &callback, bool checked = false) {
            auto checkbox = new CheckBox{ hiddenOptionsPopup, label, callback };
            checkbox->setChecked(checked);
            checkbox->setTooltip(tooltip);
            return checkbox;
        };

        new Label{ hiddenOptionsPopup, "Advanced View Options", "sans-bold" };
        m_UseShadowsCheckbox = addHiddenOptionToggle("Shadows", "Enable/Disable shadows (Shift+S)",
            [this](bool checked) {
            m_BSDFCanvas->setDrawFlag(USES_SHADOWS, checked);
        }, true);
        m_DisplayCenterAxis = addHiddenOptionToggle("Center Axis", "Show/Hide Center Axis (A)",
            [this](bool checked) {
            m_BSDFCanvas->setDrawFlag(DISPLAY_AXIS, checked);
        }, true);
        m_DisplayPredictedOutgoingAngleCheckbox = addHiddenOptionToggle("Predicted Outgoing Angle", "Show/Hide Predicted Outgoing Angle (Ctrl+I)",
            [this](bool checked) {
            m_BSDFCanvas->setDrawFlag(DISPLAY_PREDICTED_OUTGOING_ANGLE, checked);
        });
        m_DisplayDegreesCheckbox = addHiddenOptionToggle("Grid Degrees", "Show/Hide grid degrees (Shift+G)",
            [this](bool checked) { m_BSDFCanvas->grid().setShowDegrees(checked); }, true);

        new Label{ hiddenOptionsPopup , "Point Size" };
        auto pointSizeSlider = new Slider{ hiddenOptionsPopup };
        pointSizeSlider->setRange(make_pair(0.1f, 10.0f));
        pointSizeSlider->setValue(1.0f);
        pointSizeSlider->setCallback([this](float value) {
            m_BSDFCanvas->setPointSizeScale(value);
        });

        auto choseColorMapButton = new Button{ hiddenOptionsPopup, "Chose Color Map" };
        choseColorMapButton->setTooltip("Chose with which color map the data should be displayed (C)");
        choseColorMapButton->setCallback([this]() {
            toggleColorMapSelectionWindow();
        });
    }

    // mouse mode
    {
        new Label{ m_ToolWindow, "Mouse Mode", "sans-bold", 25};
        m_MouseModeSelector = new ComboBox{ m_ToolWindow, {"Rotation", "Translation", "Box Selection"} };
        m_MouseModeSelector->setCallback([this](int index) {
            m_BSDFCanvas->setMouseMode(static_cast<BSDFCanvas::MouseMode>(index));
            glfwSetCursor(mGLFWWindow, m_Cursors[index]);
        });
        m_MouseModeSelector->setTooltip("Change mouse mode to rotation (R), translation (T) or box selection (B)");

        m_Cursors[BSDFCanvas::MouseMode::ROTATE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        m_Cursors[BSDFCanvas::MouseMode::TRANSLATE] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        m_Cursors[BSDFCanvas::MouseMode::SELECTION] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
        glfwSetCursor(mGLFWWindow, m_Cursors[m_BSDFCanvas->mouseMode()]);
    }

    // grid view otpions
    {
        auto label = new Label(m_ToolWindow, "View Options", "sans-bold", 25);
        label->setTooltip(
            "Various view modes. Hover on them to learn what they do."
        );

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

        auto makeToolButton = [&](bool enabled, function<void()> callback, int icon = 0, string tooltip = "") {
            auto button = new Button{tools, "", icon};
            button->setCallback(callback);
            button->setTooltip(tooltip);
            button->setFontSize(15);
            button->setEnabled(enabled);
            return button;
        };

        makeToolButton(true, [this] { openDataSampleDialog(); }, ENTYPO_ICON_FOLDER, "Open data sample (CTRL+O)");
        makeToolButton(true, [this] { saveScreenShot(); }, ENTYPO_ICON_IMAGE, "Save image (CTRL+P)");
        makeToolButton(true, [this] { saveSelectedDataSample(); }, ENTYPO_ICON_SAVE, "Save data (CTRL+S)");
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

    setResizeCallback([this](Vector2i) { requestLayoutUpdate(); });

    requestLayoutUpdate();
    openFiles(dataSamplePaths);
}

BSDFApplication::~BSDFApplication()
{
    m_Framebuffer.free();

    for (size_t i = 0; i < BSDFCanvas::MOUSE_MODE_COUNT; i++)
    {
        glfwDestroyCursor(m_Cursors[i]);
    }
}

bool BSDFApplication::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers))
        return true;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        bool alt = modifiers & GLFW_MOD_ALT;
        // control options
        if (modifiers & SYSTEM_COMMAND_MOD)
        {
            switch (key)
            {
            case GLFW_KEY_O:
                openDataSampleDialog();
                return true;
            case GLFW_KEY_S:
                cout << "Save data\n";
                return true;
            case GLFW_KEY_P:
                saveScreenShot();
                return true;
            case GLFW_KEY_1: if (!alt) break;
            case GLFW_KEY_KP_1:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::BACK);
                return true;
            case GLFW_KEY_3: if (!alt) break;
            case GLFW_KEY_KP_3:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::RIGHT);
                return true;
            case GLFW_KEY_7: if (!alt) break;
            case GLFW_KEY_KP_7:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::DOWN);
                return true;
            case GLFW_KEY_I:
                toggleCanvasDrawFlags(DISPLAY_PREDICTED_OUTGOING_ANGLE, m_DisplayPredictedOutgoingAngleCheckbox);
                return true;
            }
        }
        else if (modifiers & GLFW_MOD_SHIFT)
        {
            switch (key)
            {
                case GLFW_KEY_S:
                    toggleCanvasDrawFlags(USES_SHADOWS, m_UseShadowsCheckbox);
                    return true;
                case GLFW_KEY_G:
                {
                    int showDegrees = !m_BSDFCanvas->grid().showDegrees();
                    m_DisplayDegreesCheckbox->setChecked(showDegrees);
                    m_BSDFCanvas->grid().setShowDegrees(showDegrees);
                    return true;
                }
                case GLFW_KEY_P:
                    toggleView(DataSample::Views::POINTS, m_SelectedDataSample, !m_SelectedDataSample->displayView(DataSample::Views::POINTS));
                    return true;
                case GLFW_KEY_I:
                    toggleView(DataSample::Views::INCIDENT_ANGLE, m_SelectedDataSample, !m_SelectedDataSample->displayView(DataSample::Views::INCIDENT_ANGLE));
                    return true;
                case GLFW_KEY_H:
                    if (m_SelectedDataSample)
                    {
                        select_highest_point(m_SelectedDataSample->pointsInfo(), m_SelectedDataSample->selectedPointsInfo(), m_SelectedDataSample->selectedPoints());
                        m_SelectedDataSample->updatePointSelection();
                        // if selection window already visible, hide it
                        if(m_SelectionInfoWindow) toggleSelectionInfoWindow();
                        // show selection window
                        toggleSelectionInfoWindow();
                    }
                    return true;
                default:
                    return false;
            }
        }
        else if (alt)
        {
            switch (key)
            {
            case GLFW_KEY_1:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::FRONT);
                return true;
            case GLFW_KEY_3:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::LEFT);
                return true;
            case GLFW_KEY_7:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::UP);
                return true;
            case GLFW_KEY_5:
                toggleToolButton(m_OrthoViewToggle);
                return true;
            }
        }
        else
        {
            switch (key)
            {
            case GLFW_KEY_ESCAPE:
                if (m_SelectedDataSample)
                {
                    deselect_all_points(m_SelectedDataSample->selectedPoints());
                    m_SelectedDataSample->updatePointSelection();
                    if (m_SelectionInfoWindow) toggleSelectionInfoWindow();
                }
                return true;
            case GLFW_KEY_Q:
                setVisible(false);
                return true;
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4: case GLFW_KEY_5:
            case GLFW_KEY_6: case GLFW_KEY_7: case GLFW_KEY_8: case GLFW_KEY_9:
                selectDataSample(key - GLFW_KEY_1);
                return true;
            case GLFW_KEY_DELETE:
                deleteDataSample(m_SelectedDataSample);
                return true;
            case GLFW_KEY_D:
                if (m_SelectedDataSample && m_SelectedDataSample->hasSelection())
                {
                    delete_selected_points( m_SelectedDataSample->selectedPoints(),
                                            m_SelectedDataSample->rawPoints(),
                                            m_SelectedDataSample->V2D(),
                                            m_SelectedDataSample->selectedPointsInfo());
                    m_SelectedDataSample->deleteSelectedPoints();
                    if (m_SelectionInfoWindow) toggleSelectionInfoWindow();
                    selectDataSample(m_SelectedDataSample);
                    return true;
                }
                return false;
            case GLFW_KEY_UP: case GLFW_KEY_W:
                selectDataSample(selectedDataSampleIndex() - 1, false);
                return true;
            case GLFW_KEY_DOWN: case GLFW_KEY_S:
                selectDataSample(selectedDataSampleIndex() + 1, false);
                return true;
            case GLFW_KEY_ENTER:
                if (m_SelectedDataSample)
                {
                    correspondingButton(m_SelectedDataSample)->toggleView();
                    return true;
                }
                break;
            case GLFW_KEY_N:
                setDisplayAsLog(m_SelectedDataSample, false);
                return true;
            case GLFW_KEY_L:
                setDisplayAsLog(m_SelectedDataSample, true);
                return true;
            case GLFW_KEY_T: case GLFW_KEY_R: case GLFW_KEY_B:
            {
                BSDFCanvas::MouseMode mode = BSDFCanvas::MouseMode::ROTATE;
                if (key == GLFW_KEY_T) mode = BSDFCanvas::MouseMode::TRANSLATE;
                if (key == GLFW_KEY_B) mode = BSDFCanvas::MouseMode::SELECTION;
                m_MouseModeSelector->setSelectedIndex(mode);
                m_BSDFCanvas->setMouseMode(mode);
                glfwSetCursor(mGLFWWindow, m_Cursors[mode]);
                return true;
            }
            case GLFW_KEY_P:
                toggleView(DataSample::Views::PATH, m_SelectedDataSample, !m_SelectedDataSample->displayView(DataSample::Views::PATH));
                return true;
            case GLFW_KEY_G:
                toggleToolButton(m_GridViewToggle);
                return true;
            case GLFW_KEY_O: case GLFW_KEY_KP_5:
                toggleToolButton(m_OrthoViewToggle);
                return true;
            case GLFW_KEY_I:
                toggleMetadataWindow();
                return true;
            case GLFW_KEY_C:
                m_BSDFCanvas->snapToSelectionCenter();
                return true;
            case GLFW_KEY_M:
                toggleColorMapSelectionWindow();
                return true;
            case GLFW_KEY_A:
                toggleCanvasDrawFlags(DISPLAY_AXIS, m_DisplayCenterAxis);
                return true;
            case GLFW_KEY_H:
                toggleHelpWindow();
                return true;
            case GLFW_KEY_KP_1:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::FRONT);
                return true;
            case GLFW_KEY_KP_3:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::LEFT);
                return true;
            case GLFW_KEY_KP_7:
                m_BSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::UP);
                return true;
            case GLFW_KEY_KP_ADD:
            case GLFW_KEY_KP_SUBTRACT:
                if (m_SelectedDataSample)
                {
                    move_selection_along_path(key == GLFW_KEY_KP_ADD, m_SelectedDataSample->selectedPoints());
                    m_SelectedDataSample->updatePointSelection();
                    // if selection window already visible, hide it
                    if (m_SelectionInfoWindow) toggleSelectionInfoWindow();
                    // show selection window
                    toggleSelectionInfoWindow();
                }
                return true;
            default:
                return false;
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

    try {
        while (true) {
            auto newDataSample = m_DataSamplesToAdd.tryPop();
            if (!newDataSample->dataSample)
            {
                auto errorMsgDialog = new MessageDialog(this, MessageDialog::Type::Warning, "Error",
                    newDataSample->errorMsg, "Retry", "Cancel", true);
                errorMsgDialog->setCallback([this](int index) {
                    if (index == 0) { openDataSampleDialog(); }
                });
            }
            else
            {
                newDataSample->dataSample->initShaders();
                newDataSample->dataSample->linkDataToShaders();
                addDataSample(m_DataSamples.size(), newDataSample->dataSample);
            }
        }
    }
    catch (runtime_error) {
    }
}

void BSDFApplication::draw(NVGcontext * ctx)
{
    Screen::draw(ctx);
    /*nvgFontSize(ctx, 30.0f);
    nvgFontFace(ctx, "sans");
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, nanogui::Color(1.0f, 0.8f));
    nvgText(ctx, mSize.x() / 2, mSize.y() / 2, "Hello world", nullptr);*/
}

void BSDFApplication::updateLayout()
{
    m_3DView->setFixedSize(mSize);
    
    m_Footer->setFixedSize(Vector2i{ mSize.x(), 20 });
    for(auto& footerInfos: m_Footer->children())
        footerInfos->setFixedWidth(width() / 3);

    m_BSDFCanvas->setFixedSize(Vector2i{ mSize.x(), mSize.y() - 20 });
    m_ToolWindow->setFixedSize({ 210, 400 });

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
    vector<string> dataSamplePaths = file_dialog(
    {
        {"txt",  "Data samples"},
    }, false, true);

    openFiles(dataSamplePaths);
    // Make sure we gain focus after seleting a file to be loaded.
    glfwFocusWindow(mGLFWWindow);
}

void BSDFApplication::openFiles(const std::vector<std::string>& dataSamplePaths)
{
    for (const auto& dataSamplePath : dataSamplePaths)
    {
        m_ThreadPool.addTask([this, dataSamplePath]() {
            auto newDataSample = make_shared<DataSampleToAdd>();
            tryLoadDataSample(dataSamplePath, newDataSample);
            m_DataSamplesToAdd.push(newDataSample);
        });
    }
}

void BSDFApplication::saveSelectedDataSample()
{
    if (m_SelectedDataSample)
    {
        string path = file_dialog(
        {
            { "txt",  "Data samples" },
        }, true);

        if (path.empty())
            return;

        m_SelectedDataSample->save(path);
    }
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

void BSDFApplication::toggleWindow(nanogui::Window *& window, function<nanogui::Window*(void)> createWindow)
{
    if (window)
    {
        window->dispose();
        window = nullptr;
    }
    else
    {
        window = createWindow();
        window->requestFocus();
        requestLayoutUpdate();
    }
}

void BSDFApplication::toggleMetadataWindow()
{
    toggleWindow(m_MetadataWindow, [this]() {
        Window *window;
        if (m_SelectedDataSample)
        {
            window = new MetadataWindow(this, &m_SelectedDataSample->metadata(), [this]() { toggleMetadataWindow(); });
        }
        else
        {
            auto errorWindow = new MessageDialog(this, MessageDialog::Type::Warning, "Metadata",
                "No data sample selected.", "close");
            errorWindow->setCallback([this](int) { m_MetadataWindow = nullptr; });
            window = errorWindow;
        }
        window->center();
        return window;
    });
}

void BSDFApplication::toggleSelectionInfoWindow()
{
    toggleWindow(m_SelectionInfoWindow, [this]() {

        auto window = new Window{ this, "Selection Info" };
        window->setLayout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 5, 5 });

        auto makeSelectionInfoLabels = [this, window](const string& caption, const string& value) {
            new Label{ window, caption, "sans-bold" };
            new Label{ window, value };
        };

        unsigned int points_count = m_SelectedDataSample->selectedPointsInfo().pointsCount();
        string min_intensity = "-";
        string max_intensity = "-";
        string average_intensity = "-";
        if (points_count != 0)
        {
            min_intensity = to_string(m_SelectedDataSample->selectedPointsInfo().minIntensity());
            max_intensity = to_string(m_SelectedDataSample->selectedPointsInfo().maxIntensity());
            average_intensity = to_string(m_SelectedDataSample->selectedPointsInfo().averageIntensity());
        }

        makeSelectionInfoLabels("Points In Selection :", to_string(points_count));
        makeSelectionInfoLabels("Minimum Intensity :",   min_intensity);
        makeSelectionInfoLabels("Maximum Intensity :",   max_intensity);
        makeSelectionInfoLabels("Average Intensity :",   average_intensity);

        window->setPosition(Vector2i{width() - 200, 20});

        return window;
    });
}

void BSDFApplication::toggleHelpWindow()
{
    toggleWindow(m_HelpWindow, [this]() {
        auto window = new HelpWindow(this, [this]() {toggleHelpWindow(); });
        window->center();
        return window;
    });
}

void BSDFApplication::toggleColorMapSelectionWindow()
{
    toggleWindow(m_ColorMapSelectionWindow, [this]() {
        auto window = new ColorMapSelectionWindow{
            this,
            m_ColorMaps,
        };
        window->setCloseCallback([this]() { toggleColorMapSelectionWindow(); });
        window->setSelectionCallback([this](shared_ptr<ColorMap> colorMap) { selectColorMap(colorMap); });
        auto pos = distance(m_ColorMaps.begin(), find(m_ColorMaps.begin(), m_ColorMaps.end(), m_BSDFCanvas->colorMap()));
        window->setSelectedButton(static_cast<size_t>(pos));
        window->center();
        return dynamic_cast<Window*>(window);
    });
}

void BSDFApplication::selectColorMap(shared_ptr<ColorMap> colorMap)
{
    m_BSDFCanvas->setColorMap(colorMap);
}

int BSDFApplication::dataSampleIndex(const shared_ptr<const DataSample> dataSample) const
{
    auto pos = static_cast<size_t>(distance(m_DataSamples.begin(), find(m_DataSamples.begin(), m_DataSamples.end(), dataSample)));
    return pos >= m_DataSamples.size() ? -1 : static_cast<int>(pos);
}

void BSDFApplication::selectDataSample(int index, bool clamped)
{
    if (m_DataSamples.empty())
        return;

    if (clamped)
        index = max(0, min(static_cast<int>(m_DataSamples.size()-1), index));
    else if (index < 0 || index >= static_cast<int>(m_DataSamples.size()))
        return;

    selectDataSample(m_DataSamples[index]);
}

void BSDFApplication::selectDataSample(shared_ptr<DataSample> dataSample)
{
    // de-select previously selected button
    if (m_SelectedDataSample)
    {
        DataSampleButton* oldButton = correspondingButton(m_SelectedDataSample);
        oldButton->setIsSelected(false);
        oldButton->showPopup(false);
    }

    m_SelectedDataSample = dataSample;
    m_BSDFCanvas->selectDataSample(dataSample);
    if (m_SelectedDataSample)
    {
        auto button = correspondingButton(m_SelectedDataSample);
        button->setIsSelected(true);
        button->showPopup(true);

        m_DataSampleName->setCaption(m_SelectedDataSample->name());
        m_DataSamplePointsCount->setCaption(to_string(m_SelectedDataSample->pointsInfo().pointsCount()));
        m_DataSampleAverageHeight->setCaption(to_string(m_SelectedDataSample->pointsInfo().averageIntensity()));
    }
    else
    {
        m_DataSampleName->setCaption("-");
        m_DataSamplePointsCount->setCaption("-");
        m_DataSampleAverageHeight->setCaption("-");
    }
    
    requestLayoutUpdate();
}

void BSDFApplication::deleteDataSample(shared_ptr<DataSample> dataSample)
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return;

    // erase data sample and corresponding button
    auto button = correspondingButton(dataSample);
    button->removePopupFromParent();
    m_DataSampleButtonContainer->removeChild(index);
    
    m_BSDFCanvas->removeDataSample(dataSample);
    m_DataSamples.erase(find(m_DataSamples.begin(), m_DataSamples.end(), dataSample));

    // clear focus path and drag widget pointer, since it may refer to deleted button
    mDragWidget = nullptr;
    mDragActive = false;
    mFocusPath.clear();

    // update selected datasample, if we just deleted the selected data sample
    if (dataSample == m_SelectedDataSample)
    {
        shared_ptr<DataSample> dataSampleToSelect = nullptr;
        if (index >= static_cast<int>(m_DataSamples.size())) --index;
        if (index >= 0)
        {
            dataSampleToSelect = m_DataSamples[index];
        }
        // Make sure no button is selected
        m_SelectedDataSample = nullptr;
        selectDataSample(dataSampleToSelect);
    }
    requestLayoutUpdate();
}

void BSDFApplication::addDataSample(int index, shared_ptr<DataSample> dataSample)
{
    if (!dataSample) {
        throw invalid_argument{ "Data sample may not be null." };
    }

    string cleanName = dataSample->name();
    replace(cleanName.begin(), cleanName.end(), '_', ' ');
    auto dataSampleButton = new DataSampleButton(m_DataSampleButtonContainer, cleanName);
    dataSampleButton->setFixedHeight(30);


    dataSampleButton->setCallback([this, dataSample]() {
        selectDataSample(dataSample);
    });

    dataSampleButton->setDeleteCallback([this, dataSample]() {
        deleteDataSample(dataSample);
    });

    dataSampleButton->setToggleViewCallback([this, dataSample](bool checked) {
        int index = dataSampleIndex(dataSample);
        if (checked)    m_BSDFCanvas->addDataSample(m_DataSamples[index]);
        else            m_BSDFCanvas->removeDataSample(m_DataSamples[index]);
    });

    dataSampleButton->setViewTogglesCallback([this, dataSample, dataSampleButton](bool) {
        for (int i = 0; i != DataSample::Views::VIEW_COUNT; ++i)
        {
            DataSample::Views view = static_cast<DataSample::Views>(i);
            toggleView(view, dataSample, dataSampleButton->isViewToggled(view));
        }
    });

    dataSampleButton->setDisplayAsLogCallback([this, dataSample](bool checked) {
        setDisplayAsLog(dataSample, checked);
    });

    m_DataSamples.push_back(dataSample);
    selectDataSample(dataSample);

    // by default toggle view for the new data samples
    m_BSDFCanvas->addDataSample(m_SelectedDataSample);
}

void BSDFApplication::toggleToolButton(nanogui::Button* button)
{
    button->setPushed(!button->pushed());
    button->changeCallback()(button->pushed());
}

void BSDFApplication::toggleView(DataSample::Views view, shared_ptr<DataSample> dataSample, bool toggle)
{
    if (dataSample)
    {
        dataSample->toggleView(view, toggle);
        correspondingButton(dataSample)->toggleView(view, dataSample->displayView(view));
    }
}

void BSDFApplication::setDisplayAsLog(shared_ptr<DataSample> dataSample, bool value)
{
    if (dataSample)
    {
        dataSample->setDisplayAsLog(value);
        correspondingButton(dataSample)->setDisplayAsLog(value);
    }
}

DataSampleButton* BSDFApplication::correspondingButton(const shared_ptr<const DataSample> dataSample)
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(m_DataSampleButtonContainer->childAt(index));
}

const DataSampleButton* BSDFApplication::correspondingButton(const shared_ptr<const DataSample> dataSample) const
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(m_DataSampleButtonContainer->childAt(index));
}

void BSDFApplication::toggleCanvasDrawFlags(int flag, CheckBox *checkbox)
{
    int flags = m_BSDFCanvas->drawFlags() ^ flag;
    checkbox->setChecked(static_cast<bool>(flags & flag));
    m_BSDFCanvas->setDrawFlags(flags);
}

void BSDFApplication::tryLoadDataSample(string filePath, shared_ptr<DataSampleToAdd> dataSampleToAdd)
{
    try {
        dataSampleToAdd->dataSample = make_shared<DataSample>(filePath);
    }
    catch (exception e) {
        string errorMsg = "Could not open data sample at " + filePath + " : " + e.what();
        cerr << errorMsg << endl;
        dataSampleToAdd->errorMsg = errorMsg;
    }
}

TEKARI_NAMESPACE_END