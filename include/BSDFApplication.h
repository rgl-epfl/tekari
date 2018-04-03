#pragma once

#include <nanogui/screen.h>
#include <memory>

#include "BSDFCanvas.h"
#include "DataSampleButton.h"
#include "MetadataWindow.h"
#include "HelpWindow.h"
#include "ColorMap.h"

class BSDFApplication : public nanogui::Screen {
public:
    BSDFApplication();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void drawContents() override;
    void requestLayoutUpdate() { m_RequiresLayoutUpdate = true; }

    void openDataSampleDialog();
    void saveScreenShot();
    void toggleMetadataWindow();
    void toggleHelpWindow();
    void deleteDataSample(DataSampleButton* button);
    void selectDataSample(DataSampleButton* button);
    void selectDataSample(int index, bool clamped = true);

private:
    void toggleView(DataSample::Views view);

    void updateLayout();
    void addDataSampleButton(int index, std::shared_ptr<DataSample> dataSample);
    void refreshToolButtons();

    void toggleToolButton(nanogui::Button* button, bool needsSelectedDataSample);

    bool hasSelectedDataSample() const { return m_SelectedDataSampleIndex >= 0; }
    std::shared_ptr<DataSample> getSelectedDataSample() { return m_DataSamples[m_SelectedDataSampleIndex]; }
    const std::shared_ptr<const DataSample> getSelectedDataSample() const { return m_DataSamples[m_SelectedDataSampleIndex]; }

    bool m_RequiresLayoutUpdate = false;

    nanogui::Window* m_ToolWindow;

    // data sample scroll panel
    nanogui::VScrollPanel* m_DataSamplesScrollPanel;
    nanogui::Widget* m_ScrollContent;
    nanogui::Widget* m_DataSampleButtonContainer;

    // tool buttons
    nanogui::Button* m_HelpButton;
    nanogui::Widget* m_ViewButtonsContainer;
    nanogui::Button* m_GridViewToggle;
    nanogui::Button* m_OrthoViewToggle;

    // dialog windows
    nanogui::Window* m_MetadataWindow;
    HelpWindow* m_HelpWindow;

    BSDFCanvas *m_BSDFCanvas;

    int m_SelectedDataSampleIndex;
    std::vector<std::shared_ptr<DataSample>> m_DataSamples;
    std::vector<std::shared_ptr<ColorMap>> m_ColorMaps;

    std::string fileName;
    std::string imageName;
};