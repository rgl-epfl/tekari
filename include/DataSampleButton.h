#pragma once

#include <nanogui/widget.h>
#include <nanogui/opengl.h>
#include <nanogui/button.h>
#include <functional>
#include <memory>

#define BUTTON_RADIUS 10.0f

class DataSampleButton : public nanogui::Widget
{
public:
    DataSampleButton(nanogui::Widget* parent, const std::string &label);

    //virtual nanogui::Vector2i preferredSize(NVGcontext *ctx) const override;
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool mouseEnterEvent(const nanogui::Vector2i &p, bool enter) override;
    virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;
    virtual void performLayout(NVGcontext *ctx) override;

    void toggleView();

    bool isSelected() const { return m_IsSelected; }
    void setIsSelected(bool isSelected) { m_IsSelected = isSelected; }

    void setCallback            (std::function<void(void)> callback) { m_Callback = callback; }
    void setDeleteCallback      (std::function<void(void)> callback) { m_DeleteCallback = callback; }
    void setToggleViewCallback  (std::function<void(bool)> callback) { m_ToggleViewCallback = callback; }

    void setNormalToggleCallback    (std::function<void(bool)> callback) { m_NormalViewToggle->setChangeCallback(callback); }
    void setLogToggleCallback       (std::function<void(bool)> callback) { m_LogViewToggle->setChangeCallback(callback); }
    void setPathToggleCallback      (std::function<void(bool)> callback) { m_PathViewToggle->setChangeCallback(callback); }

    nanogui::Popup* popup()             { return m_Popup; }
    const nanogui::Popup* popup() const { return m_Popup; }

private:
    bool InToggleViewButton(const nanogui::Vector2i& p) const {
        return (p - mPos - m_ToggleViewButtonPos).squaredNorm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }
    bool InDeleteButton(const nanogui::Vector2i& p) const {
        return (p - mPos - m_DeleteButtonPos).squaredNorm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }

    std::string m_Label;
    std::string m_DisplayLabel;
    bool m_IsSelected;
    bool m_IsVisible;

    nanogui::Vector2i m_ToggleViewButtonPos;
    nanogui::Vector2i m_DeleteButtonPos;
    bool m_ToggleViewButtonHovered;
    bool m_DeleteButtonHovered;

    std::function<void(void)> m_Callback;
    std::function<void(bool)> m_ToggleViewCallback;
    std::function<void(void)> m_DeleteCallback;

    //std::shared_ptr<DataSample> m_DataSample;
    nanogui::Popup *m_Popup;
    nanogui::Button *m_NormalViewToggle;
    nanogui::Button *m_LogViewToggle;
    nanogui::Button *m_PathViewToggle;
};