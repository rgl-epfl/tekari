#pragma once

#include <nanogui/glcanvas.h>
#include <nanogui/widget.h>
#include <memory>
#include "DataSample.h"
#include "RadialGrid.h"

class BSDFCanvas : public nanogui::GLCanvas {
public:
    enum ViewAngles
    {
        FRONT, BACK, UP, DOWN, LEFT, RIGHT
    };
public:
    BSDFCanvas(Widget *parent);

    virtual bool mouseMotionEvent(const nanogui::Vector2i &p,
                                  const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;

    virtual void drawGL() override;
    virtual void draw(NVGcontext* ctx) override;

    void addDataSample(std::shared_ptr<DataSample> dataSample);
    void removeDataSample(std::shared_ptr<DataSample> dataSample);

    void setOrthoMode(bool orthoMode) { m_OrthoMode = orthoMode; }
    
    const RadialGrid& grid() const { return m_Grid; }
    RadialGrid& grid() { return m_Grid; }

    virtual void performLayout(NVGcontext *ctx) override { m_Arcball.setSize(mSize); }

    void setViewAngle(ViewAngles viewAngle);

private:
    void getMVPMatrices(nanogui::Matrix4f &model, nanogui::Matrix4f &view, nanogui::Matrix4f &proj) const;

    std::vector<std::shared_ptr<DataSample>> m_DataSamplesToDraw;
    RadialGrid m_Grid;
    nanogui::Arcball m_Arcball;

    nanogui::Vector3f m_ViewOrigin;
    nanogui::Vector3f m_ViewTarget;
    nanogui::Vector3f m_ViewUp;

    float m_Zoom;

    bool m_OrthoMode;

    // selection region
    std::pair<nanogui::Vector2i, nanogui::Vector2i> m_SelectionRegion;
};