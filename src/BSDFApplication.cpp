#include "BSDFApplication.h"

#include <algorithm>
#include <string>

using namespace nanogui;

BSDFApplication::BSDFApplication()
:   nanogui::Screen(Eigen::Vector2i(1000, 800), "BSDF Visualizer", true)
,	m_SelectedDataSampleIndex(-1)
{
	m_BSDFCanvas = new BSDFCanvas(this);
	m_BSDFCanvas->setBackgroundColor({ 50, 50, 50, 255 });

	m_ToolWindow = new Window(this, "Tools");
	m_ToolWindow->setLayout(new BoxLayout{Orientation::Vertical, Alignment::Fill, 0, 0});
	m_ToolWindow->setVisible(true);

	m_HelpButton = new Button(m_ToolWindow->buttonPanel(), "", ENTYPO_ICON_HELP);
	m_HelpButton->setCallback([this]()
	{ 
		new MessageDialog(this, MessageDialog::Type::Information, "Info",
			"BSDF Visualizer is a tool to allow easy visualization of BSDF data.",
			"close");
	});
	m_HelpButton->setFontSize(15);
	m_HelpButton->setTooltip("Information about using BSDF Vidualizer (H)");
	m_HelpButton->setPosition({20, 0});

    // Different view options for the selected data sample
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

		m_NormalViewToggle = new Button(panel, "Normal");
		m_NormalViewToggle->setFlags(Button::Flags::ToggleButton);
		m_NormalViewToggle->setTooltip("Display/Hide normal view (N)");
		m_NormalViewToggle->setChangeCallback([this](bool checked) {
			if (hasSelectedDataSample())
			{
				getSelectedDataSample()->setDisplayNormalView(checked);
			}
		});

		m_LogViewToggle = new Button(panel, "Log");
		m_LogViewToggle->setFlags(Button::Flags::ToggleButton);
		m_LogViewToggle->setTooltip("Display/Hide log view (L)");
		m_LogViewToggle->setChangeCallback([this](bool checked) {
			if (hasSelectedDataSample())
			{
				getSelectedDataSample()->setDisplayLogView(checked);
			}
		});

		m_PathViewToggle = new Button(panel, "Path");
		m_PathViewToggle->setFlags(Button::Flags::ToggleButton);
		m_PathViewToggle->setTooltip("Display/Hide path view (P)");
		m_PathViewToggle->setChangeCallback([this](bool checked) {
			if (hasSelectedDataSample())
			{
				getSelectedDataSample()->setDisplayPath(checked);
			}
		});
	}
    // grid view otpions
	{
        auto panel = new Widget(m_ToolWindow);
        panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5, 5));

		m_GridViewToggle = new Button(panel, "Grid");
		m_GridViewToggle->setFlags(Button::Flags::ToggleButton);
		m_GridViewToggle->setTooltip("Display/Hide grid (G)");
		m_GridViewToggle->setChangeCallback( [this] (bool checked) {
			m_BSDFCanvas->grid().setVisible(checked);
		});
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
        new Label(m_ToolWindow, "File", "sans-bold", 25);
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

		makeToolButton(true, [this] {
            openDataSampleDialog();
        }, ENTYPO_ICON_FOLDER, "Open data sample (CTRL+O)");

		makeToolButton(true, [this] {
            saveScreenShot();
        }, ENTYPO_ICON_IMAGE, "Save image (CTRL+P)");
		
		makeToolButton(true, [this] {
			std::cout << "Data saved\n";
		}, ENTYPO_ICON_SAVE, "Save data (CTRL+S)");

		m_InfoButton = makeToolButton(true, [this]() {
			std::string metadataInfo = hasSelectedDataSample() ?
				getSelectedDataSample()->metadata().getInfos() :
				"No data sample selected.";
			new MessageDialog(this, MessageDialog::Type::Information, "Metadata",
				metadataInfo, "close");
		}, ENTYPO_ICON_INFO, "Show selected dataset infos (I)");
    }

	// Data sample selection
	{
		m_DataSamplesScrollPanel = new VScrollPanel(m_ToolWindow);
		m_DataSamplesScrollPanel->setFixedWidth(m_ToolWindow->width());

		m_DataSamplesScrollContent = new Widget(m_DataSamplesScrollPanel);
		m_DataSamplesScrollContent->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 5, 5));
	}

	updateToolButtons();

    setResizeCallback([this](Vector2i) { requestLayoutUpdate(); });
    this->setSize(Vector2i(1024, 800));
}

bool BSDFApplication::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers))
        return true;
	if (action == GLFW_PRESS)
	{
		// control options
		if (modifiers == GLFW_MOD_CONTROL)
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
				if (!m_DataSamples.empty())
				{
					selectDataSample(key - GLFW_KEY_1);
					return true;
				}
				break;
			case GLFW_KEY_DELETE:
				if (hasSelectedDataSample())
				{
					auto buttonToDelete = m_DataSamplesScrollContent->childAt(m_SelectedDataSampleIndex);
					deleteDataSample(dynamic_cast<DataSampleButton*>(buttonToDelete));
					return true;
				}
				break;
			case GLFW_KEY_UP:
			case GLFW_KEY_DOWN:
				if (!m_DataSamples.empty())
				{
					selectDataSample(m_SelectedDataSampleIndex + (key == GLFW_KEY_UP ? -1 : 1));
					return true;
				}
				break;

			case GLFW_KEY_ENTER:
				if (hasSelectedDataSample())
				{
					auto w = m_DataSamplesScrollContent->childAt(m_SelectedDataSampleIndex);
					DataSampleButton* button = dynamic_cast<DataSampleButton*>(w);
					button->toggleView();
				}

			case GLFW_KEY_O:
				toggleToolButton(m_OrthoViewToggle, false);
				break;
			case GLFW_KEY_N:
				toggleToolButton(m_NormalViewToggle, true);
				break;
			case GLFW_KEY_L:
				toggleToolButton(m_LogViewToggle, true);
				break;
			case GLFW_KEY_P:
				toggleToolButton(m_PathViewToggle, true);
				break;
			case GLFW_KEY_G:
				toggleToolButton(m_GridViewToggle, false);
				break;
			case GLFW_KEY_I:
				m_InfoButton->callback()();
				break;
			case GLFW_KEY_H:
				m_HelpButton->callback()();
				break;
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
	m_ToolWindow->setPosition({ 0, 0 });

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
			std::shared_ptr<DataSample> newDataSample = std::make_shared<DataSample>(dataSamplePath);
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
    std::cout << "Save screenshot." << std::endl;

    // TODO ask how this is suppose to work
    nanogui::GLFramebuffer offscreenBuffer;
    offscreenBuffer.init(m_BSDFCanvas->size(), 1);
    offscreenBuffer.bind();

    m_BSDFCanvas->drawGL();

    offscreenBuffer.downloadTGA("test.tga");
    offscreenBuffer.free();
}

void BSDFApplication::selectDataSample(int index, bool clamped)
{
	if (clamped)
		index = std::max(0, std::min(static_cast<int>(m_DataSamples.size()-1), index));
	else if (index < 0 || index >= m_DataSamples.size())
		return;

	auto buttonToSelect = m_DataSamplesScrollContent->childAt(index);
	selectDataSample(dynamic_cast<DataSampleButton*>(buttonToSelect));
}

void BSDFApplication::selectDataSample(DataSampleButton* button)
{
	// de-select previously selected button
	if (hasSelectedDataSample())
	{
		auto oldButton = m_DataSamplesScrollContent->childAt(m_SelectedDataSampleIndex);
		dynamic_cast<DataSampleButton*>(oldButton)->setIsSelected(false);
	}

	// get button index
	m_SelectedDataSampleIndex = m_DataSamplesScrollContent->childIndex(button);
	// if the button gave a valid index
	if (hasSelectedDataSample())
	{
		// select button
		button->setIsSelected(true);
	}

	updateToolButtons();
	requestLayoutUpdate();
}

void BSDFApplication::deleteDataSample(DataSampleButton* button)
{
	int index = m_DataSamplesScrollContent->childIndex(button);
	if (index < 0 && index >= m_DataSamples.size())
		return;

	// erase data sample and corresponding button
	m_BSDFCanvas->removeDataSample(m_DataSamples[index]);
	m_DataSamples.erase(m_DataSamples.begin() + index);
	m_DataSamplesScrollContent->removeChild(button);
	// clear focus path, since it may contain reference to deleted button
	mFocusPath.clear();

	// select next valid one
	DataSampleButton* buttonToSelect = nullptr;
	if (index >= m_DataSamples.size()) --index;
	if (index >= 0)
	{
		buttonToSelect = dynamic_cast<DataSampleButton*>(m_DataSamplesScrollContent->childAt(index));
	}
	selectDataSample(buttonToSelect);
}

void BSDFApplication::addDataSampleButton(int index, std::shared_ptr<DataSample> dataSample)
{
	std::string cleanName = dataSample->metadata().sampleName;
	std::replace(cleanName.begin(), cleanName.end(), '_', ' ');
	auto dataSampleButton = new DataSampleButton(nullptr, cleanName);
	m_DataSamplesScrollContent->addChild(index, dataSampleButton);
	dataSampleButton->setParent(m_DataSamplesScrollContent);
	dataSampleButton->setFixedSize({ m_DataSamplesScrollContent->width(), 30 });

	dataSampleButton->setCallback([this](DataSampleButton* w) { selectDataSample(w); });
	dataSampleButton->setDeleteCallback([this, dataSampleButton](DataSampleButton* w) { deleteDataSample(w); });
	dataSampleButton->setToggleViewCallback([this](bool checked, DataSampleButton* w) {
		int index = m_DataSamplesScrollContent->childIndex(w);
		if (checked)
		{
			m_BSDFCanvas->addDataSample(m_DataSamples[index]);
		}
		else
		{
			m_BSDFCanvas->removeDataSample(m_DataSamples[index]);
		}
	});
	selectDataSample(dataSampleButton);
	// by default toggle view for the new data samples
	m_BSDFCanvas->addDataSample(getSelectedDataSample());
}

void BSDFApplication::updateToolButtons()
{
	if (!hasSelectedDataSample())
	{
		m_NormalViewToggle->setPushed(false);
		m_LogViewToggle->setPushed(false);
		m_PathViewToggle->setPushed(false);
	}
	else
	{
		m_NormalViewToggle->setPushed(getSelectedDataSample()->displayNormalView());
		m_LogViewToggle->setPushed(getSelectedDataSample()->displayLogView());
		m_PathViewToggle->setPushed(getSelectedDataSample()->displayPath());
	}

	m_NormalViewToggle->setEnabled(hasSelectedDataSample());
	m_LogViewToggle->setEnabled(hasSelectedDataSample());
	m_PathViewToggle->setEnabled(hasSelectedDataSample());
}

void BSDFApplication::toggleToolButton(nanogui::Button* button, bool needsSelectedDataSample)
{
	if (!needsSelectedDataSample || hasSelectedDataSample())
	{
		button->setPushed(!button->pushed());
		button->changeCallback()(button->pushed());
	}
}