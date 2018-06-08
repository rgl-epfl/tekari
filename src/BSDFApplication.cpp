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

#include "stb_image.h"
#include "tekari/selections.h"
#include "tekari/raw_data_processing.h"
#include "tekari/data_io.h"
#include "tekari/LightTheme.h"

#define FOOTER_HEIGHT 25

using namespace nanogui;
using namespace std;

TEKARI_NAMESPACE_BEGIN

BSDFApplication::BSDFApplication(const std::vector<std::string>& dataSamplePaths)
:   Screen(Vector2i(1200, 750), "Tekari", true, false, 8, 8, 24, 8, 2)
,   mSelectedDS(nullptr)
,	mMetadataWindow(nullptr)
,   mHelpWindow(nullptr)
,   mSelectionInfoWindow(nullptr)
,	mUnsavedDataWindow(nullptr)
,	mColorMapSelectionWindow(nullptr)
{
    // load color maps
    for (auto& p : ColorMap::PREDEFINED_MAPS)
    {
        mColorMaps.push_back(make_shared<ColorMap>(p.first, ColorMap::FOLDER_PATH + p.second));
    }

    m3DView = new Widget{this};
    m3DView->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Fill });

    // canvas
    mBSDFCanvas = new BSDFCanvas{ m3DView };
    mBSDFCanvas->setBackgroundColor({ 50, 50, 50, 255 });
    mBSDFCanvas->setSelectionCallback([this](const Matrix4f& mvp, const SelectionBox& selectionBox,
        const Vector2i& canvasSize, SelectionMode mode) {
		if (!mSelectedDS)
			return;
        
		if (selectionBox.empty())
        {
            select_closest_point(   mSelectedDS->V2D(), mSelectedDS->currH(),
                                    mSelectedDS->selectedPoints(),
                                    mvp, selectionBox.topLeft, canvasSize);
        }
        else
        {
            select_points(  mSelectedDS->V2D(), mSelectedDS->currH(),
                            mSelectedDS->selectedPoints(),
                            mvp, selectionBox, canvasSize, mode);
        }
		mSelectedDS->updatePointSelection();
		updateSelectionInfoWindow();
    });
    mBSDFCanvas->setColorMap(mColorMaps[0]);

    // Footer
    {
        mFooter = new Widget{ m3DView };
        mFooter->setLayout(new GridLayout{ Orientation::Horizontal, 3, Alignment::Fill, 5});

        auto makeFooterInfo = [this](string label) {
            auto container = new Widget{ mFooter };
            container->setLayout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill });
            container->setFixedWidth(width() / 3);
            new Label{ container, label };
            auto info = new Label{ container, "-" };
            return info;
        };

        mDataSampleName = makeFooterInfo("Data Sample Name : ");
        mDataSamplePointsCount = makeFooterInfo("Points Count : ");
        mDataSampleAverageHeight = makeFooterInfo("AverageIntensity : ");
    }

    mToolWindow = new Window(this, "Tools");
    mToolWindow->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 5, 5});
    mToolWindow->setVisible(true);
    mToolWindow->setPosition({ 20, 20 });

    mHelpButton = new Button(mToolWindow->buttonPanel(), "", ENTYPO_ICON_HELP);
    mHelpButton->setCallback([this]() { toggleHelpWindow(); });
    mHelpButton->setFontSize(15);
    mHelpButton->setTooltip("Information about using BSDF Vidualizer (H)");
    mHelpButton->setPosition({20, 0});

    // Hidden options
    {
        mHiddenOptionsButton = new PopupButton(mToolWindow->buttonPanel(), "", ENTYPO_ICON_TOOLS);
        mHiddenOptionsButton->setBackgroundColor(Color{0.4f, 0.1f, 0.1f, 1.0f});
        mHiddenOptionsButton->setTooltip("More view options");
        auto hiddenOptionsPopup = mHiddenOptionsButton->popup();
        hiddenOptionsPopup->setLayout(new GroupLayout{});

        auto addHiddenOptionToggle = [hiddenOptionsPopup](const string& label, const string& tooltip,
            const function<void(bool)> &callback, bool checked = false) {
            auto checkbox = new CheckBox{ hiddenOptionsPopup, label, callback };
            checkbox->setChecked(checked);
            checkbox->setTooltip(tooltip);
            return checkbox;
        };

        new Label{ hiddenOptionsPopup, "Advanced View Options", "sans-bold" };
        mUseShadowsCheckbox = addHiddenOptionToggle("Shadows", "Enable/Disable shadows (Shift+S)",
            [this](bool checked) {
            mBSDFCanvas->setDrawFlag(USES_SHADOWS, checked);
			mUseSpecularCheckbox->setEnabled(checked);
        }, true);
		mUseSpecularCheckbox = addHiddenOptionToggle("Specular", "Enable/Disable specular lighting",
			[this](bool checked) {
			mBSDFCanvas->setDrawFlag(USES_SPECULAR, checked);
		}, false);
        mDisplayCenterAxis = addHiddenOptionToggle("Center Axis", "Show/Hide Center Axis (A)",
            [this](bool checked) {
            mBSDFCanvas->setDrawFlag(DISPLAY_AXIS, checked);
        }, true);
        mDisplayPredictedOutgoingAngleCheckbox = addHiddenOptionToggle("Predicted Outgoing Angle", "Show/Hide Predicted Outgoing Angle (Ctrl+I)",
            [this](bool checked) {
            mBSDFCanvas->setDrawFlag(DISPLAY_PREDICTED_OUTGOING_ANGLE, checked);
        });
		addHiddenOptionToggle("Use Light Theme", "Switch from dark to light theme",
			[this](bool checked) {
			setTheme(checked ? new LightTheme{ nvgContext() } : new Theme{ nvgContext() });
			setBackground(mTheme->mWindowFillFocused);
			mHiddenOptionsButton->setBackgroundColor(checked ? Color{ 0.7f, 0.3f, 0.3f, 1.0f } : Color{ 0.4f, 0.1f, 0.1f, 1.0f });
		});

        auto pointSizeLabel = new Label{ hiddenOptionsPopup , "Point Size" };
		pointSizeLabel->setTooltip("Changes the point size based on a arbitrary heuristic (also distance dependent)");
        auto pointSizeSlider = new Slider{ hiddenOptionsPopup };
        pointSizeSlider->setRange(make_pair(0.1f, 10.0f));
        pointSizeSlider->setValue(1.0f);
        pointSizeSlider->setCallback([this](float value) {
            mBSDFCanvas->setPointSizeScale(value);
        });

        auto choseColorMapButton = new Button{ hiddenOptionsPopup, "Chose Color Map" };
        choseColorMapButton->setTooltip("Chose with which color map the data should be displayed (M)");
        choseColorMapButton->setCallback([this]() {
            toggleColorMapSelectionWindow();
        });

		new Label{ hiddenOptionsPopup, "Grid Options", "sans-bold" };
		auto gridColorLabel = new Label{ hiddenOptionsPopup, "Color" };
		gridColorLabel->setTooltip("Chose in witch color the grid should be displayed");
		auto colorwheel = new ColorWheel{ hiddenOptionsPopup, mBSDFCanvas->grid().color() };

		auto gridAlphaLabel = new Label{ hiddenOptionsPopup, "Alpha" };
		gridAlphaLabel->setTooltip("Chose the grid transparency (left = fully transparent, right = fully opaque)");
		auto gridAlphaSlider = new Slider{ hiddenOptionsPopup };
		gridAlphaSlider->setRange({ 0.0f, 1.0f });
		gridAlphaSlider->setCallback([this](float value) { mBSDFCanvas->grid().setAlpha(value); });

		gridAlphaSlider->setValue(mBSDFCanvas->grid().alpha());

		colorwheel->setCallback([gridAlphaSlider, this](const Color& value) {
			mBSDFCanvas->grid().setColor(value);
			mBSDFCanvas->grid().setAlpha(gridAlphaSlider->value());
		});

		mDisplayDegreesCheckbox = addHiddenOptionToggle("Grid Degrees", "Show/Hide grid degrees (Shift+G)",
			[this](bool checked) { mBSDFCanvas->grid().setShowDegrees(checked); }, true);
    }

    // mouse mode
    {
        auto mouseModeLabel = new Label{ mToolWindow, "Mouse Mode", "sans-bold"};
		mouseModeLabel->setTooltip("Change mouse mode to rotation (R), translation (T) or box selection (B)");
        mMouseModeSelector = new ComboBox{ mToolWindow, {"Rotation", "Translation", "Box Selection"} };
        mMouseModeSelector->setCallback([this](int index) {
            mBSDFCanvas->setMouseMode(static_cast<BSDFCanvas::MouseMode>(index));
            glfwSetCursor(mGLFWWindow, mCursors[index]);
        });

        mCursors[BSDFCanvas::MouseMode::ROTATE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        mCursors[BSDFCanvas::MouseMode::TRANSLATE] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        mCursors[BSDFCanvas::MouseMode::SELECTION] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
        glfwSetCursor(mGLFWWindow, mCursors[mBSDFCanvas->mouseMode()]);
    }

    // grid view otpions
    {
        auto label = new Label(mToolWindow, "View Options", "sans-bold");
        label->setTooltip(
            "Various view modes. Hover on them to learn what they do."
        );

        auto panel = new Widget(mToolWindow);
        panel->setLayout(new GridLayout(Orientation::Horizontal, 3, Alignment::Fill));

        mGridViewToggle = new Button(panel, "Grid");
        mGridViewToggle->setFlags(Button::Flags::ToggleButton);
        mGridViewToggle->setTooltip("Display/Hide grid (G)");
        mGridViewToggle->setChangeCallback( [this] (bool checked) { mBSDFCanvas->grid().setVisible(checked); });
        mGridViewToggle->setPushed(true);

        mOrthoViewToggle = new Button(panel, "Ortho");
        mOrthoViewToggle->setFlags(Button::Flags::ToggleButton);
        mOrthoViewToggle->setTooltip("Enable/Disable orthogonal projection (O)");
        mOrthoViewToggle->setChangeCallback([this](bool checked) { mBSDFCanvas->setOrthoMode(checked); });
        mOrthoViewToggle->setPushed(false);

        auto backgroundColorPopupButton = new PopupButton(panel, "", ENTYPO_ICON_BUCKET);

        backgroundColorPopupButton->setFontSize(15);
        backgroundColorPopupButton->setChevronIcon(0);
        backgroundColorPopupButton->setTooltip("Background Color");

        // Background color popup
        {
            auto popup = backgroundColorPopupButton->popup();
            popup->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 10});

            new Label{popup, "Background Color"};
            auto colorwheel = new ColorWheel{popup, mBSDFCanvas->backgroundColor()};

            colorwheel->setCallback([this](const Color& value) { mBSDFCanvas->setBackgroundColor(value); });
        }
    }

    // Open, save screenshot, save data
    {
        new Label(mToolWindow, "Data Samples", "sans-bold");
        auto tools = new Widget{ mToolWindow };
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
        mDataSamplesScrollPanel = new VScrollPanel{ mToolWindow };

        mScrollContent = new Widget{ mDataSamplesScrollPanel };
        mScrollContent->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill));

        mDataSampleButtonContainer = new Widget{ mScrollContent };
        mDataSampleButtonContainer->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 0));
    }

	// load application icon
	{
		constexpr unsigned int icon_count = 5;
		const char* icon_paths[icon_count] =
		{
			"../resources/icons/tekari_icon_16x16.png",
			"../resources/icons/tekari_icon_32x32.png",
			"../resources/icons/tekari_icon_64x64.png",
			"../resources/icons/tekari_icon_128x128.png",
			"../resources/icons/tekari_icon_256x256.png"
		};

		GLFWimage icons[icon_count];
		unsigned int i;
		for (i = 0; i < icon_count; i++)
		{
			int numChanels;
			icons[i].pixels = stbi_load(icon_paths[i], &icons[i].width, &icons[i].height, &numChanels, 0);
			if (!icons[i].pixels)
			{
				cout << "Warning : unable to load Tekari's icons\n";
				break;
			}
		}
		if (i == icon_count)
			glfwSetWindowIcon(mGLFWWindow, icon_count, icons);

		for (unsigned int j = 0; j < i; j++)
		{
			stbi_image_free(icons[j].pixels);
		}
	}

	setResizeCallback([this](Vector2i) { requestLayoutUpdate(); });
	
	setBackground(mTheme->mWindowFillFocused);

    requestLayoutUpdate();
    openFiles(dataSamplePaths);
}

BSDFApplication::~BSDFApplication()
{
    mFramebuffer.free();

    for (size_t i = 0; i < BSDFCanvas::MOUSE_MODE_COUNT; i++)
    {
        glfwDestroyCursor(mCursors[i]);
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
            case GLFW_KEY_A:
				if (!mSelectedDS)
					return false;

                select_all_points(mSelectedDS->selectedPoints());
                mSelectedDS->updatePointSelection();
				updateSelectionInfoWindow();
                return true;
            case GLFW_KEY_P:
                saveScreenShot();
                return true;
            case GLFW_KEY_1: if (!alt) return false;
            case GLFW_KEY_KP_1:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::BACK);
                return true;
            case GLFW_KEY_3: if (!alt) return false;
            case GLFW_KEY_KP_3:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::RIGHT);
                return true;
            case GLFW_KEY_7: if (!alt) return false;
            case GLFW_KEY_KP_7:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::DOWN);
                return true;
            case GLFW_KEY_I:
                toggleCanvasDrawFlags(DISPLAY_PREDICTED_OUTGOING_ANGLE, mDisplayPredictedOutgoingAngleCheckbox);
                return true;
            }
        }
        else if (modifiers & GLFW_MOD_SHIFT)
        {
            switch (key)
            {
                case GLFW_KEY_S:
                    toggleCanvasDrawFlags(USES_SHADOWS, mUseShadowsCheckbox);
					mUseSpecularCheckbox->setEnabled(mUseShadowsCheckbox->checked());
                    return true;
                case GLFW_KEY_G:
                {
                    int showDegrees = !mBSDFCanvas->grid().showDegrees();
                    mDisplayDegreesCheckbox->setChecked(showDegrees);
                    mBSDFCanvas->grid().setShowDegrees(showDegrees);
                    return true;
                }
                case GLFW_KEY_P:
                    toggleView(DataSample::Views::POINTS, mSelectedDS, !mSelectedDS->displayView(DataSample::Views::POINTS));
                    return true;
                case GLFW_KEY_I:
                    toggleView(DataSample::Views::INCIDENT_ANGLE, mSelectedDS, !mSelectedDS->displayView(DataSample::Views::INCIDENT_ANGLE));
                    return true;
                case GLFW_KEY_H:
				case GLFW_KEY_L:
					if (!mSelectedDS)
						return false;
                    
					select_extreme_point( mSelectedDS->pointsStats(),
                                            mSelectedDS->selectionStats(),
                                            mSelectedDS->selectedPoints(),
                                            mSelectedDS->waveLengthIndex(),
											key == GLFW_KEY_H);
                    mSelectedDS->updatePointSelection();
					updateSelectionInfoWindow();
                    return true;
				case GLFW_KEY_1:
				case GLFW_KEY_2:
					if (!mSelectedDS || !mSelectedDS->hasSelection())
						return false;

					move_selection_along_path(key == GLFW_KEY_1, mSelectedDS->selectedPoints());
					mSelectedDS->updatePointSelection();
					updateSelectionInfoWindow();
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
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::FRONT);
                return true;
            case GLFW_KEY_3:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::LEFT);
                return true;
            case GLFW_KEY_7:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::UP);
                return true;
            case GLFW_KEY_5:
                toggleToolButton(mOrthoViewToggle);
                return true;
            }
        }
        else
        {
            switch (key)
            {
			case GLFW_KEY_F1:
				hideWindows();
				return true;
            case GLFW_KEY_ESCAPE:
				if (!mSelectedDS || !mSelectedDS->hasSelection())
					return false;
                 
				deselect_all_points(mSelectedDS->selectedPoints());
                mSelectedDS->updatePointSelection();
				updateSelectionInfoWindow();
                return true;
            case GLFW_KEY_Q:
			{
				vector<string> dsNames;
				for (const auto& ds : mDataSamples)
				{
					if (ds->dirty())
					{
						dsNames.push_back(ds->name());
					}
				}
				if (dsNames.empty()) setVisible(false);
				else toggleUnsavedDataWindow(dsNames, [this]() { setVisible(false); });
			}
				return true;
            case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4: case GLFW_KEY_5:
            case GLFW_KEY_6: case GLFW_KEY_7: case GLFW_KEY_8: case GLFW_KEY_9:
                selectDataSample(key - GLFW_KEY_1);
                return true;
            case GLFW_KEY_DELETE:
                deleteDataSample(mSelectedDS);
                return true;
            case GLFW_KEY_D:
				if (!mSelectedDS || !mSelectedDS->hasSelection())
					return false;

                delete_selected_points(
                    mSelectedDS->selectedPoints(),
                    mSelectedDS->rawPoints(),
                    mSelectedDS->V2D(),
                    mSelectedDS->selectionStats(),
					mSelectedDS->metadata()
                );

                recompute_data(
                    mSelectedDS->rawPoints(),
                    mSelectedDS->pointsStats(),
                    mSelectedDS->pathSegments(),
                    mSelectedDS->F(),
                    mSelectedDS->V2D(),
                    mSelectedDS->H(), mSelectedDS->LH(),
                    mSelectedDS->N(), mSelectedDS->LN()
                );

                mSelectedDS->linkDataToShaders();
				mSelectedDS->setDirty(true);
				correspondingButton(mSelectedDS)->setDirty(true);

				reprintFooter();
				updateSelectionInfoWindow();
				requestLayoutUpdate();
                return true;
            case GLFW_KEY_UP: case GLFW_KEY_W:
                selectDataSample(selectedDataSampleIndex() - 1, false);
                return true;
            case GLFW_KEY_DOWN: case GLFW_KEY_S:
                selectDataSample(selectedDataSampleIndex() + 1, false);
                return true;
            case GLFW_KEY_ENTER:
				if (!mSelectedDS)
					return false;
				correspondingButton(mSelectedDS)->toggleView();
                return true;
            case GLFW_KEY_L:
				toggleLogView(mSelectedDS);
                return true;
            case GLFW_KEY_T: case GLFW_KEY_R: case GLFW_KEY_B:
            {
                BSDFCanvas::MouseMode mode = BSDFCanvas::MouseMode::ROTATE;
                if (key == GLFW_KEY_T) mode = BSDFCanvas::MouseMode::TRANSLATE;
                if (key == GLFW_KEY_B) mode = BSDFCanvas::MouseMode::SELECTION;
                mMouseModeSelector->setSelectedIndex(mode);
                mBSDFCanvas->setMouseMode(mode);
                glfwSetCursor(mGLFWWindow, mCursors[mode]);
                return true;
            }
            case GLFW_KEY_P:
                toggleView(DataSample::Views::PATH, mSelectedDS, !mSelectedDS->displayView(DataSample::Views::PATH));
                return true;
            case GLFW_KEY_G:
                toggleToolButton(mGridViewToggle);
                return true;
            case GLFW_KEY_O: case GLFW_KEY_KP_5:
                toggleToolButton(mOrthoViewToggle);
                return true;
            case GLFW_KEY_I:
                toggleMetadataWindow();
                return true;
            case GLFW_KEY_C:
                mBSDFCanvas->snapToSelectionCenter();
                return true;
            case GLFW_KEY_M:
                toggleColorMapSelectionWindow();
                return true;
            case GLFW_KEY_A:
                toggleCanvasDrawFlags(DISPLAY_AXIS, mDisplayCenterAxis);
                return true;
            case GLFW_KEY_H:
                toggleHelpWindow();
                return true;
            case GLFW_KEY_KP_1:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::FRONT);
                return true;
            case GLFW_KEY_KP_3:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::LEFT);
                return true;
            case GLFW_KEY_KP_7:
                mBSDFCanvas->setViewAngle(BSDFCanvas::ViewAngles::UP);
                return true;
            case GLFW_KEY_KP_ADD:
            case GLFW_KEY_KP_SUBTRACT:
				if (!mSelectedDS || !mSelectedDS->hasSelection())
					return false;

                move_selection_along_path(key == GLFW_KEY_KP_ADD, mSelectedDS->selectedPoints());
                mSelectedDS->updatePointSelection();
				updateSelectionInfoWindow();
                return true;
            default:
                return false;
            }
        }
    }
    return false;
}

void BSDFApplication::drawContents() {
    if (mRequiresLayoutUpdate)
    {
        updateLayout();
        mRequiresLayoutUpdate = false;
    }

    try {
        while (true) {
            auto newDataSample = mDataSamplesToAdd.tryPop();
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
                addDataSample(newDataSample->dataSample);
            }
        }
    }
    catch (runtime_error) {
    }
}

void BSDFApplication::updateLayout()
{
    m3DView->setFixedSize(mSize);

    mFooter->setFixedSize(Vector2i{ mSize.x(), FOOTER_HEIGHT });
    for(auto& footerInfos: mFooter->children())
        footerInfos->setFixedWidth(width() / mFooter->children().size());

    mBSDFCanvas->setFixedSize(Vector2i{ mSize.x(), mSize.y() - FOOTER_HEIGHT });
    mToolWindow->setFixedSize({ 210, 400 });

    mDataSamplesScrollPanel->setFixedHeight(
        mToolWindow->height() - mDataSamplesScrollPanel->position().y()
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
	mThreadPool.addTask([this]() {
		vector<string> dataSamplePaths = file_dialog({ { "txt",  "Data samples" } }, false, true);
		openFiles(dataSamplePaths);
		// Make sure we gain focus after seleting a file to be loaded.
		glfwFocusWindow(mGLFWWindow);
	});
}

void BSDFApplication::openFiles(const std::vector<std::string>& dataSamplePaths)
{
    for (const auto& dataSamplePath : dataSamplePaths)
    {
        mThreadPool.addTask([this, dataSamplePath]() {
            auto newDataSample = make_shared<DataSampleToAdd>();
            tryLoadDataSample(dataSamplePath, newDataSample);
            mDataSamplesToAdd.push(newDataSample);
        });
    }
}

void BSDFApplication::saveSelectedDataSample()
{
	if (!mSelectedDS)
		return;
    
	string path = file_dialog(
    {
        { "txt",  "Data samples" },
    }, true);

    if (path.empty())
        return;

    save_data_sample(path, mSelectedDS->rawPoints(), mSelectedDS->metadata());
	mSelectedDS->setDirty(false);
	correspondingButton(mSelectedDS)->setDirty(false);
}

void BSDFApplication::saveScreenShot()
{
	string screenshotName = file_dialog(
	{
		{ "tga", "TGA images" }
	}, true);

	if (screenshotName.empty())
		return;
		
	if (mFramebuffer.ready())
	{
		mFramebuffer.free();
	}
	int viewPortWidth = static_cast<int>(mPixelRatio * width());
	int viewPortHeight = static_cast<int>(mPixelRatio * height());
	mFramebuffer.init(Vector2i{ viewPortWidth, viewPortHeight }, 1);
	glViewport(0, 0, viewPortWidth, viewPortHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	mFramebuffer.bind();
	drawAll();
	mFramebuffer.downloadTGA(screenshotName);
	mFramebuffer.release();
}

void BSDFApplication::toggleWindow(Window *& window, function<Window*(void)> createWindow)
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
		window->setVisible(!mDistractionFreeMode);
		requestLayoutUpdate();
    }
}

void BSDFApplication::toggleMetadataWindow()
{
    toggleWindow(mMetadataWindow, [this]() {
        Window *window;
        if (mSelectedDS)
        {
            window = new MetadataWindow(this, &mSelectedDS->metadata(), [this]() { toggleMetadataWindow(); });
        }
        else
        {
			auto errorWindow = new MessageDialog{ this, MessageDialog::Type::Warning, "Metadata",
				"No data sample selected.", "close" };
            errorWindow->setCallback([this](int) { mMetadataWindow = nullptr; });
            window = errorWindow;
        }
        window->center();
        return window;
    });
}

void BSDFApplication::updateSelectionInfoWindow()
{
	if (mSelectionInfoWindow) toggleSelectionInfoWindow();
	toggleSelectionInfoWindow();
}

void BSDFApplication::toggleSelectionInfoWindow()
{
	// if we are trying to toggle the selection window without a selection, just return
	if (!mSelectionInfoWindow &&
		(!mSelectedDS || mSelectedDS->selectionStats().pointsCount() == 0))
		return;

    toggleWindow(mSelectionInfoWindow, [this]() {

        auto window = new Window{ this, "Selection Info" };
        window->setLayout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 5, 5 });

        auto makeSelectionInfoLabels = [this, window](const string& caption, const string& value) {
            new Label{ window, caption, "sans-bold" };
            new Label{ window, value };
        };
		
        makeSelectionInfoLabels("Points In Selection :", to_string(mSelectedDS->selectionStats().pointsCount()));
        makeSelectionInfoLabels("Minimum Intensity :", to_string(mSelectedDS->selectionMinIntensity()));
        makeSelectionInfoLabels("Maximum Intensity :", to_string(mSelectedDS->selectionMaxIntensity()));
        makeSelectionInfoLabels("Average Intensity :", to_string(mSelectedDS->selectionAverageIntensity()));

        window->setPosition(Vector2i{width() - 200, 20});

        return window;
    });
}

void BSDFApplication::toggleUnsavedDataWindow(const vector<string>& dataSampleNames, function<void(void)> continueCallback)
{
	if (dataSampleNames.empty())
		return;

	toggleWindow(mUnsavedDataWindow, [this, &dataSampleNames, continueCallback]() {
		ostringstream errorMsg;
		errorMsg << dataSampleNames[0];
		for (size_t i = 1; i < dataSampleNames.size(); ++i)
			errorMsg << " and " << dataSampleNames[i];

		errorMsg << (dataSampleNames.size() == 1 ? " has " : " have ");
		errorMsg << "some unsaved changed. Are you sure you want to continue ?";

		auto window = new MessageDialog{ this, MessageDialog::Type::Warning, "Unsaved Changes",
			errorMsg.str(), "Cancel", "Continue", true };

		window->setCallback([this, continueCallback](int i) {
			if (i != 0)
				continueCallback();
			mUnsavedDataWindow = nullptr;
		});
		window->center();
		return window;
	});
}

void BSDFApplication::toggleHelpWindow()
{
    toggleWindow(mHelpWindow, [this]() {
        auto window = new HelpWindow(this, [this]() {toggleHelpWindow(); });
        window->center();
        return window;
    });
}

void BSDFApplication::toggleColorMapSelectionWindow()
{
    toggleWindow(mColorMapSelectionWindow, [this]() {
        auto window = new ColorMapSelectionWindow{ this, mColorMaps };
        window->setCloseCallback([this]() { toggleColorMapSelectionWindow(); });
        window->setSelectionCallback([this](shared_ptr<ColorMap> colorMap) { selectColorMap(colorMap); });
        auto pos = distance(mColorMaps.begin(), find(mColorMaps.begin(), mColorMaps.end(), mBSDFCanvas->colorMap()));
        window->setSelectedButton(static_cast<size_t>(pos));
        window->center();
        return dynamic_cast<Window*>(window);
    });
}

void BSDFApplication::selectColorMap(shared_ptr<ColorMap> colorMap)
{
    mBSDFCanvas->setColorMap(colorMap);
}

int BSDFApplication::dataSampleIndex(const shared_ptr<const DataSample> dataSample) const
{
    auto pos = static_cast<size_t>(distance(mDataSamples.begin(), find(mDataSamples.begin(), mDataSamples.end(), dataSample)));
    return pos >= mDataSamples.size() ? -1 : static_cast<int>(pos);
}

void BSDFApplication::selectDataSample(int index, bool clamped)
{
    if (mDataSamples.empty())
        return;

    if (clamped)
        index = max(0, min(static_cast<int>(mDataSamples.size()-1), index));
    else if (index < 0 || index >= static_cast<int>(mDataSamples.size()))
        return;

    selectDataSample(mDataSamples[index]);
}

void BSDFApplication::selectDataSample(shared_ptr<DataSample> dataSample)
{
	// if we select the same data sample, nothing to do
	if (dataSample == mSelectedDS)
		return;

    // de-select previously selected button
    if (mSelectedDS)
    {
        DataSampleButton* oldButton = correspondingButton(mSelectedDS);
        oldButton->setSelected(false);
        oldButton->showPopup(false);
    }
	
    mSelectedDS = dataSample;
    mBSDFCanvas->selectDataSample(dataSample);

	reprintFooter();
	updateSelectionInfoWindow();
	requestLayoutUpdate();

	if (!mSelectedDS) // if no data sample is selected, we can stop there
		return;
 
	auto button = correspondingButton(mSelectedDS);
    button->setSelected(true);

	int buttonAbsY = button->absolutePosition()[1];
	int scrollAbsY = mDataSamplesScrollPanel->absolutePosition()[1];
	int buttonH = button->height();
	int scrollH = mDataSamplesScrollPanel->height();

	float scroll = mDataSamplesScrollPanel->scroll();
	if (buttonAbsY < scrollAbsY)
	{
		scroll = static_cast<float>(button->position()[1]) / mDataSampleButtonContainer->height();
	}
	else if (buttonAbsY + buttonH > scrollAbsY + scrollH)
	{
		scroll = static_cast<float>(button->position()[1]) / (mDataSampleButtonContainer->height() - buttonH);
	}
	mDataSamplesScrollPanel->setScroll(scroll);
}

void BSDFApplication::deleteDataSample(shared_ptr<DataSample> dataSample)
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return;

    // erase data sample and corresponding button
    auto button = correspondingButton(dataSample);
    button->removePopupFromParent();
    mDataSampleButtonContainer->removeChild(index);

    mBSDFCanvas->removeDataSample(dataSample);
    mDataSamples.erase(find(mDataSamples.begin(), mDataSamples.end(), dataSample));

    // clear focus path and drag widget pointer, since it may refer to deleted button
    mDragWidget = nullptr;
    mDragActive = false;
    mFocusPath.clear();

    // update selected datasample, if we just deleted the selected data sample
    if (dataSample == mSelectedDS)
    {
        shared_ptr<DataSample> dataSampleToSelect = nullptr;
        if (index >= static_cast<int>(mDataSamples.size())) --index;
        if (index >= 0)
        {
            dataSampleToSelect = mDataSamples[index];
        }
        // Make sure no button is selected
        mSelectedDS = nullptr;
        selectDataSample(dataSampleToSelect);
    }
    requestLayoutUpdate();
}

void BSDFApplication::addDataSample(shared_ptr<DataSample> dataSample)
{
    if (!dataSample) {
        throw invalid_argument{ "Data sample may not be null." };
    }

    string cleanName = dataSample->name();
    replace(cleanName.begin(), cleanName.end(), '_', ' ');
	auto dataSampleButton = new DataSampleButton{ mDataSampleButtonContainer, cleanName,
		dataSample->isSpectral(), dataSample->maxWaveLengthIndex() };
    dataSampleButton->setFixedHeight(30);

    dataSampleButton->setCallback([this, dataSample]() { selectDataSample(dataSample); });

	dataSampleButton->setWaveLengthSliderCallback([this, dataSample](unsigned int waveLengthIndex) {
		dataSample->setWaveLengthIndex(waveLengthIndex);
		reprintFooter();
	});

    dataSampleButton->setDeleteCallback([this, dataSample]() {
		if (dataSample->dirty())
		{
			toggleUnsavedDataWindow({ dataSample->name() }, [this, dataSample]() { deleteDataSample(dataSample); });
		}
		else {
			deleteDataSample(dataSample);
		}
    });

    dataSampleButton->setToggleViewCallback([this, dataSample](bool checked) {
        int index = dataSampleIndex(dataSample);
        if (checked)    mBSDFCanvas->addDataSample(mDataSamples[index]);
        else            mBSDFCanvas->removeDataSample(mDataSamples[index]);
    });

    dataSampleButton->setViewTogglesCallback([this, dataSample, dataSampleButton](bool) {
        for (int i = 0; i != DataSample::Views::VIEW_COUNT; ++i)
        {
            DataSample::Views view = static_cast<DataSample::Views>(i);
            toggleView(view, dataSample, dataSampleButton->isViewToggled(view));
        }
    });

    dataSampleButton->setDisplayAsLogCallback([this, dataSample](bool /* unused */) {
		dataSample->toggleLogView();
    });

    mDataSamples.push_back(dataSample);
    selectDataSample(dataSample);

    // by default toggle view for the new data samples
    mBSDFCanvas->addDataSample(mSelectedDS);
}

void BSDFApplication::toggleToolButton(Button* button)
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

void BSDFApplication::toggleLogView(shared_ptr<DataSample> dataSample)
{
    if (dataSample)
    {
        dataSample->toggleLogView();
        correspondingButton(dataSample)->toggleLogView();
    }
}

DataSampleButton* BSDFApplication::correspondingButton(const shared_ptr<const DataSample> dataSample)
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(mDataSampleButtonContainer->childAt(index));
}

const DataSampleButton* BSDFApplication::correspondingButton(const shared_ptr<const DataSample> dataSample) const
{
    int index = dataSampleIndex(dataSample);
    if (index == -1)
        return nullptr;
    return dynamic_cast<DataSampleButton*>(mDataSampleButtonContainer->childAt(index));
}

void BSDFApplication::toggleCanvasDrawFlags(int flag, CheckBox *checkbox)
{
    int flags = mBSDFCanvas->drawFlags() ^ flag;
    checkbox->setChecked(static_cast<bool>(flags & flag));
    mBSDFCanvas->setDrawFlags(flags);
}

void BSDFApplication::reprintFooter()
{
	mDataSampleName->setCaption			(!mSelectedDS ? "-" : mSelectedDS->name());
	mDataSamplePointsCount->setCaption	(!mSelectedDS ? "-" : to_string(mSelectedDS->pointsStats().pointsCount()));
	mDataSampleAverageHeight->setCaption(!mSelectedDS ? "-" : to_string(mSelectedDS->averageIntensity()));
}

void BSDFApplication::hideWindows()
{
	mDistractionFreeMode = !mDistractionFreeMode;

	auto toggleVisibility = [this](Window* w) { if (w) w->setVisible(!mDistractionFreeMode); };

	toggleVisibility(mToolWindow);
	toggleVisibility(mMetadataWindow);
	toggleVisibility(mHelpWindow);
	toggleVisibility(mColorMapSelectionWindow);
	toggleVisibility(mSelectionInfoWindow);
	toggleVisibility(mUnsavedDataWindow);
}

void BSDFApplication::tryLoadDataSample(string filePath, shared_ptr<DataSampleToAdd> dataSampleToAdd)
{
    try {
        shared_ptr<DataSample> ds = make_shared<DataSample>();

		size_t pos = filePath.find_last_of("\\/");
		string fileName = filePath.substr(pos+1, fileName.length());
		cout << "====================== Loading " << fileName << " ======================\n";
        load_data_sample(filePath,
                         ds->rawPoints(),
                         ds->V2D(),
                         ds->selectedPoints(),
                         ds->metadata());

        recompute_data( ds->rawPoints(),
                        ds->pointsStats(),
                        ds->pathSegments(),
                        ds->F(),
                        ds->V2D(),
                        ds->H(), ds->LH(),
                        ds->N(), ds->LN());
        dataSampleToAdd->dataSample = ds;
		cout << "================== Finished loading " << fileName << " =================\n";
    }
    catch (exception e) {
        string errorMsg = "Could not open data sample at " + filePath + " : " + e.what();
        cerr << errorMsg << endl;
        dataSampleToAdd->errorMsg = errorMsg;
    }
}

TEKARI_NAMESPACE_END