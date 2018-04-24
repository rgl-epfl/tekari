#include "tekari/BSDFCanvas.h"

#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <string>

#include "tekari/DataSample.h"

using namespace nanogui;
using namespace std;

TEKARI_NAMESPACE_BEGIN


const Vector3f BSDFCanvas::VIEW_ORIGIN{ 0, 0, 4 };
const Vector3f BSDFCanvas::VIEW_UP{ 0, 1, 0 };
const Vector3f BSDFCanvas::VIEW_RIGHT{ 1, 0, 0 };
const Matrix4f BSDFCanvas::VIEW{ lookAt(VIEW_ORIGIN, Vector3f{ 0.0f,0.0f,0.0f }, VIEW_UP) };


BSDFCanvas::BSDFCanvas(Widget *parent)
:   GLCanvas(parent)
,   m_Translation(0, 0, 0)
,	m_Zoom(0)
,	m_OrthoMode(false)
,   m_SelectionRegion(make_pair(Vector2i(0,0), Vector2i(0,0)))
,   m_UsesShadows(true)
,   m_DisplayAxis(true)
{
    m_Arcball.setState(Quaternionf(Eigen::AngleAxisf(M_PI / 4, Vector3f::UnitX())));
}

bool BSDFCanvas::mouseMotionEvent(const Vector2i &p,
                              const Vector2i &rel,
                              int button, int modifiers) {
    if (GLCanvas::mouseMotionEvent(p, rel, button, modifiers))
        return true;
    
    if (button == GLFW_MOUSE_BUTTON_2)
    {
        m_Arcball.motion(p);
        return true;
    }
    else if (button == GLFW_MOUSE_BUTTON_3)
    {
        m_SelectionRegion.second = p;
    }
    else if (button == GLFW_MOUSE_BUTTON_5)
    {
        float moveSpeed = 0.04f / (m_Zoom + 10.1f);
        Vector3f translation = m_Arcball.matrix().block<3,3>(0,0).inverse() * (-rel[0] * moveSpeed * VIEW_RIGHT + rel[1] * moveSpeed * VIEW_UP);
        m_Translation += translation;
        return true;
    }
    return false;
}

bool BSDFCanvas::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (GLCanvas::mouseButtonEvent(p, button, down, modifiers))
        return true;

    if (button == GLFW_MOUSE_BUTTON_1)
    {
        m_Arcball.button(p, down);
        return true;
    }
    else if (button == GLFW_MOUSE_BUTTON_2)
    {
        if (!down && m_SelectedDataSample)
        {
            Matrix4f model = m_Arcball.matrix() * translate(-m_Translation);
            Matrix4f proj = getProjectionMatrix();

            Matrix4f mvp = proj * VIEW * model;

            SelectionBox selectionBox = getSelectionBox();
            
            DataSample::SelectionMode           mode = DataSample::SelectionMode::STANDARD;
            if (modifiers & GLFW_MOD_SHIFT)     mode = DataSample::SelectionMode::ADD;
            else if (modifiers & GLFW_MOD_ALT)  mode = DataSample::SelectionMode::SUBTRACT;

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

bool BSDFCanvas::scrollEvent(const Vector2i &p, const Vector2f &rel)
{
    if (!GLCanvas::scrollEvent(p, rel))
    {
        m_Zoom += rel[1] * 0.2f;
        m_Zoom = min(10.0f, max(-10.0f, m_Zoom));
    }
    return true;
}

void BSDFCanvas::drawGL(NVGcontext* ctx) {
    using namespace nanogui;

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

    for (const auto& dataSample: m_DataSamplesToDraw)
    {
        dataSample->drawGL(VIEW_ORIGIN, model, VIEW, proj, m_UsesShadows, m_DisplayAxis, m_ColorMap);
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
        dir = M_PI;
    case DOWN:
        m_Arcball.setState(Quaternionf(Eigen::AngleAxisf(-M_PI * 0.5f + dir, Vector3f::UnitX())));
        break;
    case LEFT:
        dir = M_PI;
    case RIGHT:
        m_Arcball.setState(Quaternionf(Eigen::AngleAxisf(- M_PI * 0.5f + dir, Vector3f::UnitY())));
        break;
    case BACK:
        dir = M_PI;
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