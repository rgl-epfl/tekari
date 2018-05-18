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
    void requestLayoutUpdate() { mRequiresLayoutUpdate = true; }

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
    int selectedDataSampleIndex() const { return dataSampleIndex(mSelectedDataSample); }

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
    bool mRequiresLayoutUpdate = false;

    nanogui::Window* mToolWindow;
    nanogui::Widget* m3DView;

    nanogui::PopupButton* mHiddenOptionsButton;
    nanogui::CheckBox* mUseShadowsCheckbox;
    nanogui::CheckBox* mDisplayCenterAxis;
    nanogui::CheckBox* mDisplayDegreesCheckbox;
    nanogui::CheckBox* mDisplayPredictedOutgoingAngleCheckbox;

    // footer
    nanogui::Widget* mFooter;
    nanogui::Label* mDataSampleName;
    nanogui::Label* mDataSamplePointsCount;
    nanogui::Label* mDataSampleAverageHeight;

    // data sample scroll panel
    nanogui::VScrollPanel* mDataSamplesScrollPanel;
    nanogui::Widget* mScrollContent;
    nanogui::Widget* mDataSampleButtonContainer;

    // tool buttons
    nanogui::Button* mHelpButton;
    nanogui::Button* mGridViewToggle;
    nanogui::Button* mOrthoViewToggle;

    // dialog windows
    nanogui::Window* mMetadataWindow;
    nanogui::Window* mHelpWindow;
    nanogui::Window* mColorMapSelectionWindow;
    nanogui::Window* mSelectionInfoWindow;

    // canvas
    BSDFCanvas *mBSDFCanvas;

    // cursors and mouse mode
    nanogui::ComboBox *mMouseModeSelector;
    GLFWcursor* mCursors[BSDFCanvas::MOUSE_MODE_COUNT];

    std::vector<std::shared_ptr<DataSample>> mDataSamples;
    std::shared_ptr<DataSample> mSelectedDataSample;
    std::vector<std::shared_ptr<ColorMap>> mColorMaps;

    // offscreen buffer
    nanogui::GLFramebuffer mFramebuffer;

    SharedQueue<std::shared_ptr<DataSampleToAdd>> mDataSamplesToAdd;

    // threadpool
    ThreadPool<8> mThreadPool;
};

TEKARI_NAMESPACE_END