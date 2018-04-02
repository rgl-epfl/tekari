#pragma once

#include <nanogui/nanogui.h>
#include <memory>
#include "DataSample.h"
#include "RadialGrid.h"

class BSDFCanvas : public nanogui::GLCanvas {
public:
    BSDFCanvas(Widget *parent);

    virtual bool mouseMotionEvent(const nanogui::Vector2i &p,
                                  const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
    virtual bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;

    virtual void drawGL() override;

	void addDataSample(std::shared_ptr<DataSample> dataSample);
	void removeDataSample(std::shared_ptr<DataSample> dataSample);

	void setOrthoMode(bool orthoMode) { m_OrthoMode = orthoMode; }
    
    const RadialGrid& grid() const { return m_Grid; }
    RadialGrid& grid() { return m_Grid; }

    virtual void performLayout(NVGcontext *ctx) override { m_Arcball.setSize(mSize); }

private:
    std::vector<std::shared_ptr<DataSample>> m_DataSamplesToDraw;
    RadialGrid m_Grid;
    nanogui::Arcball m_Arcball;

    nanogui::Vector3f m_ViewOrigin;
    nanogui::Vector3f m_ViewTarget;
    nanogui::Vector3f m_ViewUp;

	float m_Zoom;

	bool m_OrthoMode;
};