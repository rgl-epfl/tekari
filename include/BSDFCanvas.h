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

    virtual void performLayout(NVGcontext *ctx) override { m_Arcball.setSize(mSize); }
    virtual void drawGL() override;
    virtual void draw(NVGcontext* ctx) override;

    void selectDataSample(std::shared_ptr<DataSample> dataSample) { m_SelectedDataSample = dataSample; }
    void addDataSample(std::shared_ptr<DataSample> dataSample);
    void removeDataSample(std::shared_ptr<DataSample> dataSample);

    const RadialGrid& grid() const { return m_Grid; }
    RadialGrid& grid() { return m_Grid; }

    void setOrthoMode(bool orthoMode) { m_OrthoMode = orthoMode; }
    void setViewAngle(ViewAngles viewAngle);
    void getSelectionBox(nanogui::Vector2i &topLeft, nanogui::Vector2i &size) const;
    void setSelectionCallback(std::function<void(const nanogui::Matrix4f&, const nanogui::Vector2i&,
        const nanogui::Vector2i&, const nanogui::Vector2i&, DataSample::SelectionMode)> callback) { m_SelectCallback = callback; }

    bool usesShadows() const { return m_UsesShadows; }
    void setUsesShadows(bool usesShadows) { m_UsesShadows = usesShadows; }

    void setColorMap(std::shared_ptr<ColorMap> colorMap) { m_ColorMap = colorMap; }
    const std::shared_ptr<const ColorMap> colorMap() const { return m_ColorMap; }

private:
    void getMVPMatrices(nanogui::Matrix4f &model, nanogui::Matrix4f &view, nanogui::Matrix4f &proj) const;

    std::vector<std::shared_ptr<DataSample>> m_DataSamplesToDraw;
    std::shared_ptr<DataSample> m_SelectedDataSample;
    RadialGrid m_Grid;
    nanogui::Arcball m_Arcball;

    nanogui::Vector3f m_ViewOrigin;
    nanogui::Vector3f m_ViewTarget;
    nanogui::Vector3f m_ViewUp;

    float m_Zoom;

    bool m_OrthoMode;

    // selection region
    std::pair<nanogui::Vector2i, nanogui::Vector2i> m_SelectionRegion;
    std::function<void(const nanogui::Matrix4f&, const nanogui::Vector2i&, const nanogui::Vector2i&,
        const nanogui::Vector2i&, DataSample::SelectionMode)> m_SelectCallback;

    // global state for sample display
    bool m_UsesShadows;
    std::shared_ptr<ColorMap> m_ColorMap;
};