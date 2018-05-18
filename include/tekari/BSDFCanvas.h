#pragma once

#include <nanogui/glcanvas.h>
#include <nanogui/widget.h>
#include <nanogui/label.h>
#include <nanogui/window.h>
#include <memory>
#include "common.h"
#include "Axis.h"
#include "RadialGrid.h"
#include "selections.h"
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
    enum MouseMode
    {
        ROTATE,
        TRANSLATE,
        SELECTION,
        MOUSE_MODE_COUNT
    };

private:
    // view state constants
    static const Vector3f VIEW_ORIGIN;
    static const Vector3f VIEW_UP;
    static const Vector3f VIEW_RIGHT;
    static const Matrix4f VIEW;

public:
    BSDFCanvas(nanogui::Widget *parent);

    // nanogui specific methods
    virtual bool mouseMotionEvent(const Vector2i &p,
                                  const Vector2i &rel,
                                  int button, int modifiers) override;
    virtual bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool scrollEvent(const Vector2i &p, const Vector2f &rel) override;
    virtual void performLayout(NVGcontext*) override { mArcball.setSize(mSize); }
    virtual void draw(NVGcontext* ctx) override;
    virtual void drawGL() override;

    // data sample addition/removale/selection
    void selectDataSample(std::shared_ptr<DataSample> dataSample);
    void addDataSample(std::shared_ptr<DataSample> dataSample);
    void removeDataSample(std::shared_ptr<DataSample> dataSample);

    void snapToSelectionCenter();


    void setOrthoMode(bool orthoMode) { mOrthoMode = orthoMode; }
    void setViewAngle(ViewAngles viewAngle);
    void setSelectionCallback(std::function<void(const Matrix4f&, const SelectionBox&,
        const Vector2i&, SelectionMode)> callback) { mSelectCallback = callback; }

    // Setters/Getters
    const RadialGrid& grid() const { return mGrid; }
    RadialGrid& grid() { return mGrid; }

    int drawFlags() const { return mDrawFlags; }
    void setDrawFlags(int flags) { mDrawFlags = flags; }
    void setDrawFlag(int flag, bool state) { mDrawFlags = state ? mDrawFlags | flag : mDrawFlags & ~flag; }

    void setColorMap(std::shared_ptr<ColorMap> colorMap) { mColorMap = colorMap; }
    const std::shared_ptr<const ColorMap> colorMap() const { return mColorMap; }

    void setPointSizeScale(float pointSizeScale) { mPointSizeScale = pointSizeScale; }

    void setMouseMode(MouseMode mode) { mMouseMode = mode; }
    MouseMode mouseMode() const { return mMouseMode; }

private:
    SelectionBox getSelectionBox() const;
    Matrix4f getProjectionMatrix() const;
    int rotationMouseButton(bool dragging) const;
    int translationMouseButton(bool dragging) const;
    int selectionMouseButton(bool dragging) const;

    // data samples
    std::vector<std::shared_ptr<DataSample>>    mDataSamplesToDraw;
    std::shared_ptr<DataSample>                 mSelectedDataSample;

    RadialGrid          mGrid;
    nanogui::Arcball    mArcball;

    // view state
    Vector3f mTranslation;
    float mZoom;
    float mPointSizeScale;
    bool mOrthoMode;
    MouseMode mMouseMode;

    // selection
    std::pair<Vector2i, Vector2i> mSelectionRegion;
    std::function<void(const Matrix4f&, const SelectionBox&,
        const Vector2i&, SelectionMode)> mSelectCallback;

    // global state for sample display
    int mDrawFlags;
    std::shared_ptr<ColorMap> mColorMap;

};

TEKARI_NAMESPACE_END