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
,   m_Translation(0, 0, 0)
,	m_Zoom(0)
,   m_PointSizeScale(1.0f)
,	m_OrthoMode(false)
,   m_SelectionRegion(make_pair(Vector2i(0,0), Vector2i(0,0)))
,   m_DrawFlags(DISPLAY_AXIS | USES_SHADOWS)
,   m_MouseMode(ROTATE)
{
    m_Arcball.setState(Quaternionf(Eigen::AngleAxisf(static_cast<float>(M_PI / 4.0), Vector3f::UnitX())));
}

bool BSDFCanvas::mouseMotionEvent(const Vector2i &p,
                              const Vector2i &rel,
                              int button, int modifiers) {
    if (GLCanvas::mouseMotionEvent(p, rel, button, modifiers))
        return true;
    
    if (button == rotationMouseButton(true))
    {
        m_Arcball.motion(p);
        return true;
    }
    else if (button == selectionMouseButton(true))
    {
        m_SelectionRegion.second = p;
    }
    else if (button == translationMouseButton(true))
    {
        float moveSpeed = 0.04f / (m_Zoom + MAX_ZOOM + 0.1f);
        Vector3f translation = m_Arcball.matrix().block<3,3>(0,0).inverse() * (-rel[0] * moveSpeed * VIEW_RIGHT + rel[1] * moveSpeed * VIEW_UP);
        m_Translation += translation;
        return true;
    }
    return false;
}

bool BSDFCanvas::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (GLCanvas::mouseButtonEvent(p, button, down, modifiers))
        return true;

    if (button == rotationMouseButton(false))
    {
        m_Arcball.button(p, down);
        return true;
    }
    else if (button == selectionMouseButton(false))
    {
        if (!down && m_SelectedDataSample)
        {
            Matrix4f model = m_Arcball.matrix() * translate(-m_Translation);
            Matrix4f proj = getProjectionMatrix();

            Matrix4f mvp = proj * VIEW * model;

            SelectionBox selectionBox = getSelectionBox();
            
            SelectionMode                       mode = SelectionMode::STANDARD;
            if (modifiers & GLFW_MOD_SHIFT)     mode = SelectionMode::ADD;
            else if (modifiers & GLFW_MOD_ALT)  mode = SelectionMode::SUBTRACT;

            m_SelectCallback(mvp, selectionBox, mSize, mode);
            m_SelectionRegion = make_pair(Vector2i(0, 0), Vector2i(0, 0));
        }
        else
        {
            m_SelectionRegion = make_pair(p, p);
        }
        return true;
    }
    return false;
}
int BSDFCanvas::rotationMouseButton(bool dragging) const
{
    return BUTTON_MAPPINGS[dragging][m_MouseMode];
}
int BSDFCanvas::translationMouseButton(bool dragging) const
{
    return BUTTON_MAPPINGS[dragging][(m_MouseMode + 2) % MOUSE_MODE_COUNT];
}
int BSDFCanvas::selectionMouseButton(bool dragging) const
{
    return BUTTON_MAPPINGS[dragging][(m_MouseMode + 1) % MOUSE_MODE_COUNT];
}

bool BSDFCanvas::scrollEvent(const Vector2i &p, const Vector2f &rel)
{
    if (!GLCanvas::scrollEvent(p, rel))
    {
        m_Zoom += rel[1] * 0.2f;
        m_Zoom = min(MAX_ZOOM, max(MIN_ZOOM, m_Zoom));
    }
    return true;
}

void BSDFCanvas::draw(NVGcontext* ctx)
{
    GLCanvas::draw(ctx);

    Matrix4f model = m_Arcball.matrix() * translate(-m_Translation);
    Matrix4f proj = getProjectionMatrix();

    m_Grid.draw(ctx, mSize, model, VIEW, proj);

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
    Matrix4f model = m_Arcball.matrix() * translate(-m_Translation);
    Matrix4f proj = getProjectionMatrix();

    float pointSizeFactor = (m_Zoom - MIN_ZOOM) / (MAX_ZOOM - MIN_ZOOM);
    glPointSize(pointSizeFactor * pointSizeFactor * pointSizeFactor * m_PointSizeScale);
    for (const auto& dataSample: m_DataSamplesToDraw)
    {
        dataSample->drawGL(VIEW_ORIGIN, model, VIEW, proj, m_DrawFlags, m_ColorMap);
    }
    m_Grid.drawGL(model, VIEW, proj);
}

void BSDFCanvas::selectDataSample(shared_ptr<DataSample> dataSample) {
    m_SelectedDataSample = dataSample;
}

void BSDFCanvas::addDataSample(shared_ptr<DataSample> dataSample)
{
    if (find(m_DataSamplesToDraw.begin(), m_DataSamplesToDraw.end(), dataSample) == m_DataSamplesToDraw.end())
    {
        m_DataSamplesToDraw.push_back(dataSample);
    }
}
void BSDFCanvas::removeDataSample(shared_ptr<DataSample> dataSample)
{
    auto dataSampleToErase = find(m_DataSamplesToDraw.begin(), m_DataSamplesToDraw.end(), dataSample);
    if (dataSampleToErase != m_DataSamplesToDraw.end())
    {
        m_DataSamplesToDraw.erase(dataSampleToErase);
    }
}

void BSDFCanvas::snapToSelectionCenter()
{
    m_Translation = !m_SelectedDataSample ? Vector3f{ 0.0f, 0.0f, 0.0f } :
                                            m_SelectedDataSample->selectionCenter();
}

void BSDFCanvas::setViewAngle(ViewAngles viewAngle)
{
    float dir = 0.0f;
    switch (viewAngle)
    {
    case UP:
        dir = (float)M_PI;
    case DOWN:
        m_Arcball.setState(Quaternionf(Eigen::AngleAxisf(-M_PI * 0.5f + dir, Vector3f::UnitX())));
        break;
    case LEFT:
        dir = (float)M_PI;
    case RIGHT:
        m_Arcball.setState(Quaternionf(Eigen::AngleAxisf(- M_PI * 0.5f + dir, Vector3f::UnitY())));
        break;
    case BACK:
        dir = (float)M_PI;
    case FRONT:
        m_Arcball.setState(Quaternionf(Eigen::AngleAxisf(dir, Vector3f::UnitY())));
        break;
    }
}

nanogui::Matrix4f BSDFCanvas::getProjectionMatrix() const
{
    float near = 0.01f, far = 100.0f;
    float zoomFactor = (m_Zoom + 10.0f) / 20.0f + 0.01f;
    float sizeRatio = (float)mSize.x() / (float)mSize.y();
    if (m_OrthoMode)
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
    res.topLeft = { min(m_SelectionRegion.first[0], m_SelectionRegion.second[0]),
                    min(m_SelectionRegion.first[1], m_SelectionRegion.second[1]) };
    res.size    = { abs(m_SelectionRegion.first[0] - m_SelectionRegion.second[0]),
                    abs(m_SelectionRegion.first[1] - m_SelectionRegion.second[1]) };
    return res;
}

TEKARI_NAMESPACE_END