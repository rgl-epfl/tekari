#pragma once

#include <nanogui/glcanvas.h>
#include <nanogui/widget.h>
#include <nanogui/label.h>
#include <nanogui/window.h>
#include <memory>
#include "common.h"
#include "Axis.h"
#include "RadialGrid.h"
#include "SelectionBox.h"
#include "DataSample.h"

TEKARI_NAMESPACE_BEGIN

class BSDFCanvas : public nanogui::GLCanvas
{
public:
    // usefull types
    enum ViewAngles
    {
        FRONT, BACK, UP, DOWN, LEFT, RIGHT
    };

private:
    // view state constants
    static const nanogui::Vector3f VIEW_ORIGIN;
    static const nanogui::Vector3f VIEW_UP;
    static const nanogui::Vector3f VIEW_RIGHT;
    static const nanogui::Matrix4f VIEW;

public:
    BSDFCanvas(Widget *parent);

    // nanogui specific methods
    virtual bool mouseMotionEvent(const nanogui::Vector2i &p,
                                  const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
    virtual void performLayout(NVGcontext *ctx) override { m_Arcball.setSize(mSize); }
    virtual void drawGL(NVGcontext* ctx) override;

    // data sample addition/removale/selection
    void selectDataSample(std::shared_ptr<DataSample> dataSample);
    void addDataSample(std::shared_ptr<DataSample> dataSample);
    void removeDataSample(std::shared_ptr<DataSample> dataSample);

    void snapToSelectionCenter();


    void setOrthoMode(bool orthoMode) { m_OrthoMode = orthoMode; }
    void setViewAngle(ViewAngles viewAngle);
    void setSelectionCallback(std::function<void(const nanogui::Matrix4f&, const SelectionBox&,
        const nanogui::Vector2i&, DataSample::SelectionMode)> callback) { m_SelectCallback = callback; }

    // Setters/Getters
    const RadialGrid& grid() const { return m_Grid; }
    RadialGrid& grid() { return m_Grid; }
    bool usesShadows() const { return m_UsesShadows; }
    void setUsesShadows(bool usesShadows) { m_UsesShadows = usesShadows; }
    bool displayAxis() const { return m_DisplayAxis; }
    void setDisplayAxis(bool displayAxis) { m_DisplayAxis = displayAxis; }
    void setColorMap(std::shared_ptr<ColorMap> colorMap) { m_ColorMap = colorMap; }
    const std::shared_ptr<const ColorMap> colorMap() const { return m_ColorMap; }

private:
    SelectionBox getSelectionBox() const;
    nanogui::Matrix4f getProjectionMatrix() const;

    // data samples
    std::vector<std::shared_ptr<DataSample>>    m_DataSamplesToDraw;
    std::shared_ptr<DataSample>                 m_SelectedDataSample;

    RadialGrid          m_Grid;
    nanogui::Arcball    m_Arcball;

    // view state
    nanogui::Vector3f m_Translation;
    float m_Zoom;
    bool m_OrthoMode;

    // selection
    std::pair<nanogui::Vector2i, nanogui::Vector2i> m_SelectionRegion;
    std::function<void(const nanogui::Matrix4f&, const SelectionBox&,
        const nanogui::Vector2i&, DataSample::SelectionMode)> m_SelectCallback;

    // global state for sample display
    bool m_DisplayAxis;
    bool m_UsesShadows;
    std::shared_ptr<ColorMap> m_ColorMap;

};

TEKARI_NAMESPACE_END