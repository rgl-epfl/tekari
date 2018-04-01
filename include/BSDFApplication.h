#pragma once

#include <nanogui/nanogui.h>
#include <memory>

#include "BSDFCanvas.h"

class BSDFApplication : public nanogui::Screen {
public:
    BSDFApplication();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    void drawContents() override;
    void requestLayoutUpdate() { m_RequiresLayoutUpdate = true; }

    void openDataSampleDialog();
    void saveScreenShot();
	void deleteDataSample(int index);
	void selectDataSample(int index);

private:
    void updateLayout();
	void addDataSampleButton(int index, std::shared_ptr<DataSample> dataSample);

    bool m_RequiresLayoutUpdate = false;

    nanogui::Window* m_ToolWindow;
	nanogui::VScrollPanel* m_DataSamplesScrollPanel;
	nanogui::Widget* m_DataSamplesScrollContent;

    BSDFCanvas *m_BSDFCanvas;

	std::shared_ptr<DataSample> m_CurrentDataSample;
	std::vector<std::shared_ptr<DataSample>> m_DataSamples;

    std::string fileName;
    std::string imageName;
};