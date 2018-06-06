#include "tekari/BSDFCanvas.h"

#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <string>

#include "tekari/DataSample.h"

#define MAX_ZOOM 10.0f
#define MIN_ZOOM -MAX_ZOOM

using namespace nanogui;
using namespace std;


TEKARI_NAMESPACE_BEGIN

int BUTTON_MAPPINGS[2][BSDFCanvas::MOUSE_MODE_COUNT] =
{
    { GLFW_MOUSE_BUTTON_1, GLFW_MOUSE_BUTTON_2, GLFW_MOUSE_BUTTON_3 },
    { GLFW_MOUSE_BUTTON_2, GLFW_MOUSE_BUTTON_3, GLFW_MOUSE_BUTTON_5 }
};

const Vector3f BSDFCanvas::VIEW_ORIGIN{ 0, 0, 4 };
const Vector3f BSDFCanvas::VIEW_UP{ 0, 1, 0 };
const Vector3f BSDFCanvas::VIEW_RIGHT{ 1, 0, 0 };
const Matrix4f BSDFCanvas::VIEW{ lookAt(VIEW_ORIGIN, Vector3f{ 0.0f,0.0f,0.0f }, VIEW_UP) };


BSDFCanvas::BSDFCanvas(Widget *parent)
:   GLCanvas(parent)
,   mTranslation(0, 0, 0)
,	mZoom(0)
,   mPointSizeScale(1.0f)
,	mOrthoMode(false)
,   mSelectionRegion(make_pair(Vector2i(0,0), Vector2i(0,0)))
,   mDrawFlags(DISPLAY_AXIS | USES_SHADOWS)
,   mMouseMode(ROTATE)
{
    mArcball.setState(Quaternionf(Eigen::AngleAxisf(static_cast<float>(M_PI / 4.0), Vector3f::UnitX())));
}

bool BSDFCanvas::mouseMotionEvent(const Vector2i &p,
                              const Vector2i &rel,
                              int button, int modifiers) {
    if (GLCanvas::mouseMotionEvent(p, rel, button, modifiers))
        return true;
    if (!focused())
        return false;
    
    if (button == rotationMouseButton(true))
    {
        mArcball.motion(p);
        return true;
    }
    else if (button == selectionMouseButton(true))
    {
        mSelectionRegion.second = p;
    }
    else if (button == translationMouseButton(true))
    {
        float moveSpeed = 0.04f / (mZoom + MAX_ZOOM + 0.1f);
        Vector3f translation = mArcball.matrix().block<3,3>(0,0).inverse() * (-rel[0] * moveSpeed * VIEW_RIGHT + rel[1] * moveSpeed * VIEW_UP);
        mTranslation += translation;
        return true;
    }
    return false;
}

bool BSDFCanvas::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (GLCanvas::mouseButtonEvent(p, button, down, modifiers))
        return true;
    if (!focused() && !down)
        return false;

	// Whenever we click on the canvas, we request focus (no matter the button)
	if (down)
		requestFocus();

    if (button == rotationMouseButton(false))
    {
        mArcball.button(p, down);
        return true;
    }
    else if (button == selectionMouseButton(false))
    {
        if (!down && mSelectedDataSample)
        {
            Matrix4f model = mArcball.matrix() * translate(-mTranslation);
            Matrix4f proj = getProjectionMatrix();

            Matrix4f mvp = proj * VIEW * model;

            SelectionBox selectionBox = getSelectionBox();
            
            SelectionMode                       mode = SelectionMode::STANDARD;
            if (modifiers & GLFW_MOD_SHIFT)     mode = SelectionMode::ADD;
            else if (modifiers & GLFW_MOD_ALT)  mode = SelectionMode::SUBTRACT;

            mSelectCallback(mvp, selectionBox, mSize, mode);
            mSelectionRegion = make_pair(Vector2i(0, 0), Vector2i(0, 0));
        }
        else
        {
            mSelectionRegion = make_pair(p, p);
        }
        return true;
    }
    return false;
}
int BSDFCanvas::rotationMouseButton(bool dragging) const
{
    return BUTTON_MAPPINGS[dragging][mMouseMode];
}
int BSDFCanvas::translationMouseButton(bool dragging) const
{
    return BUTTON_MAPPINGS[dragging][(mMouseMode + 2) % MOUSE_MODE_COUNT];
}
int BSDFCanvas::selectionMouseButton(bool dragging) const
{
    return BUTTON_MAPPINGS[dragging][(mMouseMode + 1) % MOUSE_MODE_COUNT];
}

bool BSDFCanvas::scrollEvent(const Vector2i &p, const Vector2f &rel)
{
    if (!GLCanvas::scrollEvent(p, rel))
    {
        mZoom += rel[1] * 0.2f;
        mZoom = min(MAX_ZOOM, max(MIN_ZOOM, mZoom));
    }
    return true;
}

void BSDFCanvas::draw(NVGcontext* ctx)
{
    GLCanvas::draw(ctx);

    Matrix4f model = mArcball.matrix() * translate(-mTranslation);
    Matrix4f proj = getProjectionMatrix();

    mGrid.draw(ctx, mSize, model, VIEW, proj);

    // draw selection region
    SelectionBox selectionBox = getSelectionBox();
    nvgBeginPath(ctx);
    nvgRect(ctx, selectionBox.topLeft.x(), selectionBox.topLeft.y(),
        selectionBox.size.x(), selectionBox.size.y());
    nvgStrokeColor(ctx, Color(1.0f, 1.0f));
    nvgStroke(ctx);
    nvgFillColor(ctx, Color(1.0f, 0.1f));
    nvgFill(ctx);
}

void BSDFCanvas::drawGL() {
    Matrix4f model = mArcball.matrix() * translate(-mTranslation);
    Matrix4f proj = getProjectionMatrix();

    float pointSizeFactor = (mZoom - MIN_ZOOM) / (MAX_ZOOM - MIN_ZOOM);
    glPointSize(pointSizeFactor * pointSizeFactor * pointSizeFactor * mPointSizeScale);
    for (const auto& dataSample: mDataSamplesToDraw)
    {
        dataSample->drawGL(VIEW_ORIGIN, model, VIEW, proj, mDrawFlags, mColorMap);
    }
    mGrid.drawGL(model, VIEW, proj);
}

void BSDFCanvas::selectDataSample(shared_ptr<DataSample> dataSample) {
    mSelectedDataSample = dataSample;
}

void BSDFCanvas::addDataSample(shared_ptr<DataSample> dataSample)
{
    if (find(mDataSamplesToDraw.begin(), mDataSamplesToDraw.end(), dataSample) == mDataSamplesToDraw.end())
    {
        mDataSamplesToDraw.push_back(dataSample);
    }
}
void BSDFCanvas::removeDataSample(shared_ptr<DataSample> dataSample)
{
    auto dataSampleToErase = find(mDataSamplesToDraw.begin(), mDataSamplesToDraw.end(), dataSample);
    if (dataSampleToErase != mDataSamplesToDraw.end())
    {
        mDataSamplesToDraw.erase(dataSampleToErase);
    }
}

void BSDFCanvas::snapToSelectionCenter()
{
    mTranslation = !mSelectedDataSample ? Vector3f{ 0.0f, 0.0f, 0.0f } :
                                            mSelectedDataSample->selectionCenter();
}

void BSDFCanvas::setViewAngle(ViewAngles viewAngle)
{
    float dir = 0.0f;
    switch (viewAngle)
    {
    case UP:
        dir = (float)M_PI;
    case DOWN:
        mArcball.setState(Quaternionf(Eigen::AngleAxisf(-M_PI * 0.5f + dir, Vector3f::UnitX())));
        break;
    case LEFT:
        dir = (float)M_PI;
    case RIGHT:
        mArcball.setState(Quaternionf(Eigen::AngleAxisf(- M_PI * 0.5f + dir, Vector3f::UnitY())));
        break;
    case BACK:
        dir = (float)M_PI;
    case FRONT:
        mArcball.setState(Quaternionf(Eigen::AngleAxisf(dir, Vector3f::UnitY())));
        break;
    }
}

Matrix4f BSDFCanvas::getProjectionMatrix() const
{
    float near = 0.01f, far = 100.0f;
    float zoomFactor = (mZoom + 10.0f) / 20.0f + 0.01f;
    float sizeRatio = (float)mSize.x() / (float)mSize.y();
    if (mOrthoMode)
    {
        zoomFactor = (1.02f - zoomFactor) * 2.0f;
        return ortho(-zoomFactor * sizeRatio, zoomFactor * sizeRatio,
                     -zoomFactor, zoomFactor,
                     near, far);
    }
    else {
        const float viewAngle = 81.0f - zoomFactor * 80.0f;
        float fH = tan(viewAngle / 360.0f * M_PI) * near;
        float fW = fH * sizeRatio;
        return frustum(-fW, fW, -fH, fH, near, far);
    }
}

SelectionBox BSDFCanvas::getSelectionBox() const
{
    SelectionBox res;
    res.topLeft = { min(mSelectionRegion.first[0], mSelectionRegion.second[0]),
                    min(mSelectionRegion.first[1], mSelectionRegion.second[1]) };
    res.size    = { abs(mSelectionRegion.first[0] - mSelectionRegion.second[0]),
                    abs(mSelectionRegion.first[1] - mSelectionRegion.second[1]) };
    return res;
}

TEKARI_NAMESPACE_END