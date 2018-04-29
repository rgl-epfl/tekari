#pragma once

#include <nanogui/widget.h>
#include <nanogui/popup.h>
#include <functional>
#include <memory>

#include "DataSample.h"

TEKARI_NAMESPACE_BEGIN

class DataSampleButton : public nanogui::Widget
{
public:
    DataSampleButton(nanogui::Widget* parent, const std::string &label);

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

    void setDisplayAsLogCallback(std::function<void(bool)> callback);
    void setViewTogglesCallback(std::function<void(bool)> callback);
  
    void showPopup(bool visible) { m_Popup->setVisible(visible); }
    void removePopupFromParent() { m_Popup->parent()->removeChild(m_Popup); }

    void toggleView(DataSample::Views view, bool check);
    bool isViewToggled(DataSample::Views view);

    void setDisplayAsLog(bool value);

private:
    bool InToggleViewButton(const nanogui::Vector2i& p) const {
        return (p - mPos - m_ToggleViewButtonPos).squaredNorm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }
    bool InDeleteButton(const nanogui::Vector2i& p) const {
        return (p - mPos - m_DeleteButtonPos).squaredNorm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }

    static constexpr float BUTTON_RADIUS = 10.0f;

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
    nanogui::CheckBox* m_DisplayAsLog;
    nanogui::Button* m_ViewToggles[DataSample::Views::VIEW_COUNT];
};

TEKARI_NAMESPACE_END