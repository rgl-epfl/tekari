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

using nanogui::Widget;
using nanogui::Screen;
using nanogui::Window;
using nanogui::Button;
using nanogui::CheckBox;
using nanogui::PopupButton;
using nanogui::Label;
using nanogui::VScrollPanel;
using nanogui::ComboBox;
using nanogui::GLFramebuffer;

struct DataSampleToAdd
{
    std::string errorMsg;
    std::shared_ptr<DataSample> dataSample;
};

class BSDFApplication : public Screen {
public:
    BSDFApplication(const std::vector<std::string>& dataSamplePaths);
    ~BSDFApplication();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void drawContents() override;
    void requestLayoutUpdate() { mRequiresLayoutUpdate = true; }

    void openDataSampleDialog();
    void saveScreenShot();
    void saveSelectedDataSample();

    void toggleWindow(Window* &window, std::function<Window*(void)> createWindow);
    void toggleMetadataWindow();
    void toggleHelpWindow();
    void toggleSelectionInfoWindow();
    void toggleColorMapSelectionWindow();
    
    void selectColorMap(std::shared_ptr<ColorMap> colorMap);
    
    void deleteDataSample(std::shared_ptr<DataSample> dataSample);
    void selectDataSample(std::shared_ptr<DataSample> dataSample);
    void selectDataSample(int index, bool clamped = true);

    int dataSampleIndex(const std::shared_ptr<const DataSample> dataSample) const;
    int selectedDataSampleIndex() const { return dataSampleIndex(mSelectedDS); }

    DataSampleButton* correspondingButton(const std::shared_ptr<const DataSample> dataSample);
    const DataSampleButton* correspondingButton(const std::shared_ptr<const DataSample> dataSample) const;

private:
    void toggleView(DataSample::Views view, std::shared_ptr<DataSample> dataSample, bool toggle);
    void setDisplayAsLog(std::shared_ptr<DataSample> dataSample, bool value);

    void updateLayout();
    void addDataSample(int index, std::shared_ptr<DataSample> dataSample);

    void toggleToolButton(Button* button);

    void openFiles(const std::vector<std::string>& dataSamplePaths);
    void tryLoadDataSample(std::string filePath, std::shared_ptr<DataSampleToAdd> dataSampleToAdd);

    void toggleCanvasDrawFlags(int flag, CheckBox *checkbox);

private:
    bool mRequiresLayoutUpdate = false;

    Window* mToolWindow;
    Widget* m3DView;

    PopupButton* mHiddenOptionsButton;
    CheckBox* mUseShadowsCheckbox;
    CheckBox* mDisplayCenterAxis;
    CheckBox* mDisplayDegreesCheckbox;
    CheckBox* mDisplayPredictedOutgoingAngleCheckbox;

    // footer
    Widget* mFooter;
    Label* mDataSampleName;
    Label* mDataSamplePointsCount;
    Label* mDataSampleAverageHeight;

    // data sample scroll panel
    VScrollPanel* mDataSamplesScrollPanel;
    Widget* mScrollContent;
    Widget* mDataSampleButtonContainer;

    // tool buttons
    Button* mHelpButton;
    Button* mGridViewToggle;
    Button* mOrthoViewToggle;

    // dialog windows
    Window* mMetadataWindow;
    Window* mHelpWindow;
    Window* mColorMapSelectionWindow;
    Window* mSelectionInfoWindow;

    // canvas
    BSDFCanvas *mBSDFCanvas;

    // cursors and mouse mode
    ComboBox *mMouseModeSelector;
    GLFWcursor* mCursors[BSDFCanvas::MOUSE_MODE_COUNT];

    std::vector<std::shared_ptr<DataSample>> mDataSamples;
    std::shared_ptr<DataSample> mSelectedDS;
    std::vector<std::shared_ptr<ColorMap>> mColorMaps;

    // offscreen buffer
    GLFramebuffer mFramebuffer;

    SharedQueue<std::shared_ptr<DataSampleToAdd>> mDataSamplesToAdd;

    // threadpool
    ThreadPool<8> mThreadPool;
};

TEKARI_NAMESPACE_END