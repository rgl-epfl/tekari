#pragma once

#include "DataSample.h"

#include <nanogui/widget.h>
#include <nanogui/popup.h>
#include <functional>
#include <memory>

TEKARI_NAMESPACE_BEGIN

class DataSampleButton : public nanogui::Widget
{
public:
    DataSampleButton(nanogui::Widget* parent, const std::string &label);

    virtual bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;
    virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;
    virtual bool mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;
    virtual void performLayout(NVGcontext *ctx) override;

    void toggleView();

    bool isSelected() const { return mIsSelected; }
    void setIsSelected(bool isSelected) { mIsSelected = isSelected; }

    void setCallback            (std::function<void(void)> callback) { mCallback = callback; }
    void setDeleteCallback      (std::function<void(void)> callback) { mDeleteCallback = callback; }
    void setToggleViewCallback  (std::function<void(bool)> callback) { mToggleViewCallback = callback; }

    void setDisplayAsLogCallback(std::function<void(bool)> callback);
    void setViewTogglesCallback(std::function<void(bool)> callback);
  
    void showPopup(bool visible) { mPopup->setVisible(visible); }
    void removePopupFromParent() { mPopup->parent()->removeChild(mPopup); }

    void toggleView(DataSample::Views view, bool check);
    bool isViewToggled(DataSample::Views view);

    void setDisplayAsLog(bool value);

private:
    bool InToggleViewButton(const Vector2i& p) const {
        return (p - mPos - mToggleViewButtonPos).squaredNorm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }
    bool InDeleteButton(const Vector2i& p) const {
        return (p - mPos - mDeleteButtonPos).squaredNorm() <= BUTTON_RADIUS*BUTTON_RADIUS;
    }

    static constexpr float BUTTON_RADIUS = 10.0f;

    std::string mLabel;
    std::string mDisplayLabel;
    bool mIsSelected;
    bool mIsVisible;

    Vector2i mToggleViewButtonPos;
    Vector2i mDeleteButtonPos;
    bool mToggleViewButtonHovered;
    bool mDeleteButtonHovered;

    std::function<void(void)> mCallback;
    std::function<void(bool)> mToggleViewCallback;
    std::function<void(void)> mDeleteCallback;

    //std::shared_ptr<DataSample> mDataSample;
    nanogui::Popup *mPopup;
    nanogui::CheckBox* mDisplayAsLog;
    nanogui::Button* mViewToggles[DataSample::Views::VIEW_COUNT];
};

TEKARI_NAMESPACE_END