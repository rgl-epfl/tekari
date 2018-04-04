#pragma once

#include <nanogui/widget.h>
#include <nanogui/opengl.h>
#include <functional>

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

    void toggleView();

    bool isSelected() const { return m_IsSelected; }
    void setIsSelected(bool isSelected) { m_IsSelected = isSelected; }

    void setCallback(std::function<void(DataSampleButton*)> callback) { m_Callback = callback; }
    void setDeleteCallback(std::function<void(DataSampleButton*)> callback) { m_DeleteCallback = callback; }
    void setToggleViewCallback(std::function<void(bool, DataSampleButton*)> callback) { m_ToggleViewCallback = callback; }

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

    std::function<void(DataSampleButton*)> m_Callback;
    std::function<void(bool, DataSampleButton*)> m_ToggleViewCallback;
    std::function<void(DataSampleButton*)> m_DeleteCallback;
};