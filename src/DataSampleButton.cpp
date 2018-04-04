#include "DataSampleButton.h"

#include <iostream>
#include <functional>

#include <nanogui/opengl.h>
#include <nanogui/common.h>
#include <nanogui/popup.h>
#include <nanogui/entypo.h>

using namespace nanogui;

DataSampleButton::DataSampleButton(Widget * parent, const std::string & label)
:   Widget{ parent }
,   m_Label{ label }
,   m_DisplayLabel{ label.size() > 20 ? label.substr(0, 17) + "..." : label }
,	m_IsSelected(false)
,   m_IsVisible(true)
,   m_ToggleViewButtonPos{ 155, 15 }
,   m_DeleteButtonPos{ 155 + 2*BUTTON_RADIUS + 2, 15 }
,   m_ToggleViewButtonHovered(false)
,   m_DeleteButtonHovered(false)
{
    setTooltip(m_Label);

    Window *parentWindow = window();
    m_Popup = new Popup{ parentWindow->parent(), window() };
    m_Popup->setVisible(false);
}

//nanogui::Vector2i DataSampleButton::preferredSize(NVGcontext *ctx) const
//{
//    nvgFontSize(ctx, mFontSize);
//    nvgFontFace(ctx, "sans-bold");
//    std::string fixedSizeLabel = m_Label.size() > 20 ? m_Label.substr(0, 17) + "..." : m_Label;
//    float labelSize = nvgTextBounds(ctx, 0, 0, fixedSizeLabel.c_str(), nullptr, nullptr);
//
//    return nanogui::Vector2i(static_cast<int>(labelSize) + 15, mFontSize + 6);
//}

bool DataSampleButton::mouseButtonEvent(const Eigen::Vector2i & p, int button, bool down, int modifiers)
{
    if (Widget::mouseButtonEvent(p, button, down, modifiers)) {
        return true;
    }

    if (!mEnabled || !down) {
        return false;
    }

    if (button == GLFW_MOUSE_BUTTON_1) {
        if (InDeleteButton(p))
        {
            m_DeleteCallback(this);
            return true;
        }
        else if (InToggleViewButton(p))
        {
            toggleView();
            return true;
        }
        else if (m_Callback)
        {
            m_Callback(this);
            return true;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_2)
    {
        if (!InDeleteButton(p) && !InToggleViewButton(p))
        {
            m_Popup->setVisible(!m_Popup->visible());
            return true;
        }
    }

    return false;
}

bool DataSampleButton::mouseEnterEvent(const nanogui::Vector2i & p, bool enter)
{
    Widget::mouseEnterEvent(p, enter);
    m_DeleteButtonHovered = false;
    m_ToggleViewButtonHovered = false;
    return false;
}

bool DataSampleButton::mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers)
{
    if (Widget::mouseMotionEvent(p, rel, button, modifiers)) {
        return true;
    }

    m_DeleteButtonHovered = InDeleteButton(p);
    m_ToggleViewButtonHovered = InToggleViewButton(p);
    return false;
}

void DataSampleButton::draw(NVGcontext * ctx)
{
    float fillOpacity = m_IsSelected ? 0.3f : mMouseFocus ? 0.25f : 0.2f;
    float deleteButtonFillOpacity = m_DeleteButtonHovered ? 0.4f : 0.2f;
    float toggleViewButtonFillOpacity = m_ToggleViewButtonHovered ? 0.4f : m_IsVisible ? 0.5f : 0.2f;

    // save current nvg state
    nvgSave(ctx);
    nvgTranslate(ctx, mPos.x(), mPos.y());

    // draw background
    nvgBeginPath(ctx);
    nvgRect(ctx, 0, 0, mSize.x(), mSize.y());
    nvgFillColor(ctx, Color(1.0f, fillOpacity));
    nvgFill(ctx);
    if (m_IsSelected || mMouseFocus)
    {
        nvgStrokeColor(ctx, Color(1.0f, 0.8f));
        nvgStrokeWidth(ctx, 1.0f);
        nvgStroke(ctx);
    }

    // draw label
    nvgFontSize(ctx, 18.0f);
    nvgFontFace(ctx, "sans");
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, Color(0.0f, 0.8f));
    nvgText(ctx, 5, mSize.y() * 0.5f - 1.0f, m_DisplayLabel.c_str(), nullptr);
    nvgFillColor(ctx, Color(1.0f, 0.8f));
    nvgText(ctx, 5, mSize.y() * 0.5f, m_DisplayLabel.c_str(), nullptr);

    // font settings for icons
    nvgFontSize(ctx, BUTTON_RADIUS * 1.3f);
    nvgFontFace(ctx, "icons");
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    // draw delete button
    auto makeToolButton = [this, &ctx](float opacity, int icon, const nanogui::Vector2i& pos) {
        nvgBeginPath(ctx);
        nvgCircle(ctx, pos.x(), pos.y(), BUTTON_RADIUS);
        nvgFillColor(ctx, Color(0.0f, opacity));
        nvgFill(ctx);
        auto iconData = utf8(icon);
        nvgFillColor(ctx, Color(0.0f, 0.8f));
        nvgText(ctx, pos.x(), pos.y() - 1.0f, iconData.data(), nullptr);
        nvgFillColor(ctx, Color(1.0f, 0.8f));
        nvgText(ctx, pos.x(), pos.y(), iconData.data(), nullptr);
    };
    makeToolButton(deleteButtonFillOpacity, ENTYPO_ICON_CROSS, m_DeleteButtonPos);
    makeToolButton(toggleViewButtonFillOpacity, m_IsVisible ? ENTYPO_ICON_EYE : ENTYPO_ICON_EYE_WITH_LINE, m_ToggleViewButtonPos);

    nvgRestore(ctx);
}

void DataSampleButton::performLayout(NVGcontext * ctx)
{
    Widget::performLayout(ctx);

    const Window *parentWindow = window();

    int posY = absolutePosition().y() - parentWindow->position().y() + mSize.y() / 2;
    if (m_Popup->side() == Popup::Right)
        m_Popup->setAnchorPos(Vector2i(parentWindow->width() + 15, posY));
    else
        m_Popup->setAnchorPos(Vector2i(0 - 15, posY));
}

void DataSampleButton::toggleView()
{
    m_IsVisible = !m_IsVisible;
    m_ToggleViewCallback(m_IsVisible, this);
}