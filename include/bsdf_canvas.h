#pragma once

#include <nanogui/nanogui.h>
#include "sample_data.h"

class BSDFCanvas : public nanogui::GLCanvas {
public:
    BSDFCanvas(Widget *parent)
    :   nanogui::GLCanvas(parent)
    ,   m_SampleData("../resources/green_pvc.txt")
    ,   m_ViewOrigin(0, 2, 4)
    ,   m_ViewTarget(0, 0, 0)
    ,   m_ViewUp(0, 1, 0)
    {
    using namespace nanogui;

    // TODO: Change this
        m_Arcball.setSize(parent->size());
    }

    virtual bool mouseMotionEvent(const nanogui::Vector2i &p,
                                  const nanogui::Vector2i &rel,
                                  int button, int modifiers) override {
        if (!GLCanvas::mouseMotionEvent(p, rel, button, modifiers))
        {
            if (button == GLFW_MOUSE_BUTTON_2)
            {
                m_Arcball.motion(p);
                return true;
            }
            else if (button == GLFW_MOUSE_BUTTON_5)
            {
                m_ViewTarget += nanogui::Vector3f(-rel[0] * 0.01f, 0, -rel[1] * 0.01f);
                return true;
            }
            return false;
        }
        return true;
    }

    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
        if (!GLCanvas::mouseButtonEvent(p, button, down, modifiers))
        {
            if (button == GLFW_MOUSE_BUTTON_1)
            {
                m_Arcball.button(p, down);
                return true;
            }
            return false;
        }
    }

    virtual bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override
    {
        if (!GLCanvas::scrollEvent(p, rel))
        {
            m_ViewOrigin += (m_ViewTarget-m_ViewOrigin) * rel[1] / 10.0f;
        }
        return true;
    }

    virtual void drawGL() override {
        using namespace nanogui;

        Matrix4f view, proj;
        view = lookAt(m_ViewOrigin, m_ViewTarget, m_ViewUp);
        const float viewAngle = 30, near = 0.01, far = 100;
        float fH = std::tan(viewAngle / 360.0f * M_PI) * near;
        float fW = fH * (float) mSize.x() / (float) mSize.y();
        proj = frustum(-fW, fW, -fH, fH, near, far);

        m_SampleData.draw(m_Arcball.matrix(), view, proj);
    }

    void setShowNormalView(bool value) { m_SampleData.displayNormalView(value); }
    void setShowPath(bool value) { m_SampleData.displayPath(value); }
    void setShowLogView(bool value) { m_SampleData.displayLogView(value); }

private:
    SampleData m_SampleData;
    nanogui::Arcball m_Arcball;

    nanogui::Vector3f m_ViewOrigin;
    nanogui::Vector3f m_ViewTarget;
    nanogui::Vector3f m_ViewUp;
};