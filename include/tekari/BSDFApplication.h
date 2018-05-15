#pragma once

#include <nanogui/screen.h>
#include <memory>
#include <thread>

#include "common.h"
#include "BSDFCanvas.h"
#include "DataSampleButton.h"
#include "MetadataWindow.h"
#include "ColorMapSelectionWindow.h"
#include "HelpWindow.h"
#include "ColorMap.h"
#include "SharedQueue.h"
#include "ThreadPool.h"

TEKARI_NAMESPACE_BEGIN

struct DataSampleToAdd
{
    std::string errorMsg;
    std::shared_ptr<DataSample> dataSample;
};

class BSDFApplication : public nanogui::Screen {
public:
    BSDFApplication(const std::vector<std::string>& dataSamplePaths);
    ~BSDFApplication();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void drawContents() override;
    void draw(NVGcontext *ctx) override;
    void requestLayoutUpdate() { m_RequiresLayoutUpdate = true; }

    void openDataSampleDialog();
    void saveScreenShot();
    void saveSelectedDataSample();

    void toggleWindow(nanogui::Window* &window, std::function<nanogui::Window*(void)> createWindow);
    void toggleMetadataWindow();
    void toggleHelpWindow();
    void toggleSelectionInfoWindow();
    void toggleColorMapSelectionWindow();
    
    void selectColorMap(std::shared_ptr<ColorMap> colorMap);
    
    void deleteDataSample(std::shared_ptr<DataSample> dataSample);
    void selectDataSample(std::shared_ptr<DataSample> dataSample);
    void selectDataSample(int index, bool clamped = true);

    int dataSampleIndex(const std::shared_ptr<const DataSample> dataSample) const;
    bool hasSelectedDataSample() const                  { return m_SelectedDataSample != nullptr; }
    std::shared_ptr<DataSample> selectedDataSample()    { return m_SelectedDataSample; }
    const std::shared_ptr<const DataSample> selectedDataSample() const { return m_SelectedDataSample; }
    int selectedDataSampleIndex() const { return dataSampleIndex(m_SelectedDataSample); }

    DataSampleButton* correspondingButton(const std::shared_ptr<const DataSample> dataSample);
    const DataSampleButton* correspondingButton(const std::shared_ptr<const DataSample> dataSample) const;

private:
    void toggleView(DataSample::Views view, std::shared_ptr<DataSample> dataSample, bool toggle);
    void setDisplayAsLog(std::shared_ptr<DataSample> dataSample, bool value);

    void updateLayout();
    void addDataSample(int index, std::shared_ptr<DataSample> dataSample);

    void toggleToolButton(nanogui::Button* button);

    void openFiles(const std::vector<std::string>& dataSamplePaths);
    void tryLoadDataSample(std::string filePath, std::shared_ptr<DataSampleToAdd> dataSampleToAdd);

    void toggleCanvasDrawFlags(int flag, nanogui::CheckBox *checkbox);

private:
    bool m_RequiresLayoutUpdate = false;

    nanogui::Window* m_ToolWindow;
    nanogui::Widget* m_3DView;

    nanogui::PopupButton* m_HiddenOptionsButton;
    nanogui::CheckBox* m_UseShadowsCheckbox;
    nanogui::CheckBox* m_DisplayCenterAxis;
    nanogui::CheckBox* m_DisplayDegreesCheckbox;
    nanogui::CheckBox* m_DisplayPredictedOutgoingAngleCheckbox;

    // footer
    nanogui::Widget* m_Footer;
    nanogui::Label* m_DataSampleName;
    nanogui::Label* m_DataSamplePointsCount;
    nanogui::Label* m_DataSampleAverageHeight;

    // data sample scroll panel
    nanogui::VScrollPanel* m_DataSamplesScrollPanel;
    nanogui::Widget* m_ScrollContent;
    nanogui::Widget* m_DataSampleButtonContainer;

    // tool buttons
    nanogui::Button* m_HelpButton;
    nanogui::Button* m_GridViewToggle;
    nanogui::Button* m_OrthoViewToggle;

    // dialog windows
    nanogui::Window* m_MetadataWindow;
    nanogui::Window* m_HelpWindow;
    nanogui::Window* m_ColorMapSelectionWindow;
    nanogui::Window* m_SelectionInfoWindow;

    // canvas
    BSDFCanvas *m_BSDFCanvas;

    // cursors and mouse mode
    nanogui::ComboBox *m_MouseModeSelector;
    GLFWcursor* m_Cursors[BSDFCanvas::MOUSE_MODE_COUNT];

    std::vector<std::shared_ptr<DataSample>> m_DataSamples;
    std::shared_ptr<DataSample> m_SelectedDataSample;
    std::vector<std::shared_ptr<ColorMap>> m_ColorMaps;

    // offscreen buffer
    nanogui::GLFramebuffer m_Framebuffer;

    SharedQueue<std::shared_ptr<DataSampleToAdd>> m_DataSamplesToAdd;

    // threadpool
    ThreadPool<8> m_ThreadPool;
};

TEKARI_NAMESPACE_END