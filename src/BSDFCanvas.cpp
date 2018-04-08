#include "BSDFCanvas.h"

using namespace nanogui;

BSDFCanvas::BSDFCanvas(Widget *parent)
:   GLCanvas(parent)
,   m_ViewOrigin(0, 0, 4)
,   m_ViewTarget(0, 0, 0)
,   m_ViewUp(0, 1, 0)
,	m_Zoom(0)
,	m_OrthoMode(false)
,   m_SelectionRegion(std::make_pair(Vector2i(0,0), Vector2i(0,0)))
,   m_UsesShadows(true)
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
        Vector3f translation = Vector3f(-rel[0] * moveSpeed, rel[1] * moveSpeed, 0.0f);
        m_ViewTarget += translation;
        m_ViewOrigin += translation;
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
            if (m_SelectionRegion.first == m_SelectionRegion.second)
            {
                m_SelectedDataSample->deselectAllPoints();
                std::cout << "HERE\n";
            }
            else
            {
                Matrix4f model, view, proj, mvp;
                getMVPMatrices(model, view, proj);
                mvp = proj * view * model;

                Vector2i topLeft, size;
                getSelectionBox(topLeft, size);
            
                DataSample::SelectionMode           mode = DataSample::SelectionMode::STANDARD;
                if (modifiers & GLFW_MOD_SHIFT)     mode = DataSample::SelectionMode::ADD;
                else if (modifiers & GLFW_MOD_ALT)  mode = DataSample::SelectionMode::SUBTRACT;

                m_SelectCallback(mvp, topLeft, size, mSize, mode);

                m_SelectionRegion = std::make_pair(Vector2i(0, 0), Vector2i(0, 0));
            }
        }
        else
        {
            m_SelectionRegion = std::make_pair(p, p);
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
        m_Zoom = std::min(10.0f, std::max(-10.0f, m_Zoom));
    }
    return true;
}

void BSDFCanvas::drawGL() {
    using namespace nanogui;

    Matrix4f model, view, proj;
    getMVPMatrices(model, view, proj);

    for (const auto& dataSample: m_DataSamplesToDraw)
    {
        dataSample->drawGL(m_ViewOrigin, model, view, proj, m_UsesShadows, m_ColorMap);
    }
    m_Grid.drawGL(model, view, proj);
}

void BSDFCanvas::draw(NVGcontext* ctx)
{
    GLCanvas::draw(ctx);

    Matrix4f model, view, proj;
    getMVPMatrices(model, view, proj);

    m_Grid.draw(ctx, mSize, model, view, proj);

    // draw selection region
    Vector2i topLeft, size;
    getSelectionBox(topLeft, size);
    nvgBeginPath(ctx);
    nvgRect(ctx, topLeft.x(), topLeft.y(), size.x(), size.y());
    nvgStrokeColor(ctx, Color(1.0f, 1.0f));
    nvgStroke(ctx);
}

void BSDFCanvas::addDataSample(std::shared_ptr<DataSample> dataSample)
{
    if (std::find(m_DataSamplesToDraw.begin(), m_DataSamplesToDraw.end(), dataSample) == m_DataSamplesToDraw.end())
    {
        m_DataSamplesToDraw.push_back(dataSample);
    }
}
void BSDFCanvas::removeDataSample(std::shared_ptr<DataSample> dataSample)
{
    auto dataSampleToErase = std::find(m_DataSamplesToDraw.begin(), m_DataSamplesToDraw.end(), dataSample);
    if (dataSampleToErase != m_DataSamplesToDraw.end())
    {
        m_DataSamplesToDraw.erase(dataSampleToErase);
    }
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

void BSDFCanvas::getMVPMatrices(nanogui::Matrix4f &model, nanogui::Matrix4f &view, nanogui::Matrix4f &proj) const
{
    view = lookAt(m_ViewOrigin, m_ViewTarget, m_ViewUp);
    float near = 0.01f, far = 100.0f;
    float zoomFactor = (m_Zoom + 10.0f) / 20.0f + 0.01f;
    float sizeRatio = (float)mSize.x() / (float)mSize.y();
    if (m_OrthoMode)
    {
        zoomFactor = (1.02f - zoomFactor) * 2.0f;
        proj = ortho(-zoomFactor * sizeRatio, zoomFactor * sizeRatio,
            -zoomFactor, zoomFactor,
            near, far);
    }
    else {
        const float viewAngle = 81.0f - zoomFactor * 80.0f;
        float fH = std::tan(viewAngle / 360.0f * M_PI) * near;
        float fW = fH * sizeRatio;
        proj = frustum(-fW, fW, -fH, fH, near, far);
    }
    model = m_Arcball.matrix();
}

void BSDFCanvas::getSelectionBox(Vector2i &topLeft, Vector2i &size) const
{
    topLeft = { std::min(m_SelectionRegion.first.x(), m_SelectionRegion.second.x()),
                std::min(m_SelectionRegion.first.y(), m_SelectionRegion.second.y()) };
    size = { std::abs(m_SelectionRegion.first.x() - m_SelectionRegion.second.x()),
             std::abs(m_SelectionRegion.first.y() - m_SelectionRegion.second.y()) };
}