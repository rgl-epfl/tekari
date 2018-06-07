#include "tekari/DataSampleButton.h"

#include <iostream>
#include <functional>

#include <nanogui/opengl.h>
#include <nanogui/common.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/entypo.h>
#include <nanogui/slider.h>
#include <nanogui/button.h>
#include <nanogui/checkbox.h>

using namespace nanogui;
using namespace std;

TEKARI_NAMESPACE_BEGIN

DataSampleButton::DataSampleButton(Widget * parent, const std::string & label, bool isSpectral, unsigned int maxWaveLengthIndex)
:   Widget{ parent }
,   mLabel{ label }
,   mDisplayLabel{ label.size() > 20 ? label.substr(0, 17) + "..." : label }
,	mSelected(false)
,   mVisible(true)
,	mDirty(false)
,   mToggleViewButtonPos{ 155, 15 }
,   mDeleteButtonPos{ 155 + 2*BUTTON_RADIUS + 2, 15 }
,   mToggleViewButtonHovered(false)
,   mDeleteButtonHovered(false)
{
    setTooltip(mLabel);

    Window *parentWindow = window();

    mPopup = new Popup{ parentWindow->parent(), window() };
    mPopup->setVisible(false);
    mPopup->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Fill, 5, 5 });

    new Label{ mPopup, "View Modes" , "sans-bold", 18};
    mDisplayAsLog = new CheckBox{ mPopup, "Display as log" };

    auto buttonContainer = new Widget{ mPopup };
    buttonContainer->setLayout(new GridLayout{ Orientation::Horizontal, 4, Alignment::Fill });

    auto makeViewButton = [this, buttonContainer](const string& label, const string& tooltip, bool pushed) {
        auto button = new Button(buttonContainer, label);
        button->setFlags(Button::Flags::ToggleButton);
        button->setTooltip(tooltip);
        button->setPushed(pushed);
        return button;
    };
    mViewToggles[DataSample::Views::PATH]   = makeViewButton("Path", "Show/Hide path for this data sample (P)", false);
    mViewToggles[DataSample::Views::POINTS] = makeViewButton("Points", "Toggle points view for this data sample (Shift + P)", false);
    mViewToggles[DataSample::Views::INCIDENT_ANGLE] = makeViewButton("Incident Angle", "Show/Hide incident angle for this data sample (Shift + I)", true);

	if (isSpectral)
	{
		auto waveLengthIndexLabel = new Label{ mPopup, "Wave length index : 0", "sans-bold", 18 };
		auto waveLengthSlider = new Slider{ mPopup };
		waveLengthSlider->setRange(make_pair(0, maxWaveLengthIndex));
		waveLengthSlider->setCallback([this, waveLengthSlider, waveLengthIndexLabel](float value) {
			unsigned int waveLengthIndex = static_cast<unsigned int>(round(value));
			waveLengthSlider->setValue(waveLengthIndex);
			waveLengthIndexLabel->setCaption("Wave length index : " + to_string(waveLengthIndex));
			mWaveLengthSliderCallback(waveLengthIndex);
		});
	}
}

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
            mDeleteCallback();
            return true;
        }
        else if (InToggleViewButton(p))
        {
            toggleView();
            return true;
        }
        else if (mCallback)
        {
            mCallback();
            return true;
        }
    }

    return false;
}

bool DataSampleButton::mouseEnterEvent(const Vector2i & p, bool enter)
{
    Widget::mouseEnterEvent(p, enter);
    mDeleteButtonHovered = false;
    mToggleViewButtonHovered = false;
    return false;
}

bool DataSampleButton::mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers)
{
    if (Widget::mouseMotionEvent(p, rel, button, modifiers)) {
        return true;
    }

    mDeleteButtonHovered = InDeleteButton(p);
    mToggleViewButtonHovered = InToggleViewButton(p);
    return false;
}

void DataSampleButton::draw(NVGcontext * ctx)
{
    if (!window()->focused() && !mPopup->focused())
        mPopup->setVisible(false);

    Color fillColor =	mSelected ? Color(0.0f, 0.8f, 0.2f, 0.5f) :
						mMouseFocus ? mTheme->mButtonGradientTopFocused :
						mTheme->mButtonGradientTopUnfocused;
    float deleteButtonFillOpacity = mDeleteButtonHovered ? 0.4f : 0.2f;
    float toggleViewButtonFillOpacity = mToggleViewButtonHovered ? 0.4f : mVisible ? 0.5f : 0.2f;

    // save current nvg state
    nvgSave(ctx);
    nvgTranslate(ctx, mPos.x(), mPos.y());

    // draw background
    nvgBeginPath(ctx);
    nvgRect(ctx, 0, 0, mSize.x(), mSize.y());
    nvgFillColor(ctx, fillColor);
    nvgFill(ctx);
    if (mMouseFocus)
    {
        nvgStrokeColor(ctx, Color(1.0f, 0.8f));
        nvgStrokeWidth(ctx, 1.0f);
        nvgStroke(ctx);
    }

    // draw label
	string label = mDirty ? mDisplayLabel + "*" : mDisplayLabel;
    nvgFontSize(ctx, 18.0f);
    nvgFontFace(ctx, "sans");
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(ctx, mTheme->mTextColorShadow);
    nvgText(ctx, 5, mSize.y() * 0.5f - 1.0f, label.c_str(), nullptr);
    nvgFillColor(ctx, mTheme->mTextColor);
    nvgText(ctx, 5, mSize.y() * 0.5f, label.c_str(), nullptr);

    // font settings for icons
    nvgFontSize(ctx, BUTTON_RADIUS * 1.3f);
    nvgFontFace(ctx, "icons");
    nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

    // draw delete button
    auto makeToolButton = [this, &ctx](float opacity, int icon, const Vector2i& pos) {
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
    makeToolButton(deleteButtonFillOpacity, ENTYPO_ICON_CROSS, mDeleteButtonPos);
    makeToolButton(toggleViewButtonFillOpacity, mVisible ? ENTYPO_ICON_EYE : ENTYPO_ICON_EYE_WITH_LINE, mToggleViewButtonPos);

    nvgRestore(ctx);
}

void DataSampleButton::performLayout(NVGcontext * ctx)
{
    Widget::performLayout(ctx);

    const Window *parentWindow = window();

    int posY = absolutePosition().y() - parentWindow->position().y() + mSize.y() / 2;
    if (mPopup->side() == Popup::Right)
        mPopup->setAnchorPos(Vector2i(parentWindow->width() + 15, posY));
    else
        mPopup->setAnchorPos(Vector2i(0 - 15, posY));
}

void DataSampleButton::toggleView()
{
    mVisible = !mVisible;
    mToggleViewCallback(mVisible);
}

void DataSampleButton::toggleView(DataSample::Views view, bool check)
{
    mViewToggles[static_cast<int>(view)]->setPushed(check);
}

bool DataSampleButton::isViewToggled(DataSample::Views view)
{
    return mViewToggles[static_cast<int>(view)]->pushed();
}

void DataSampleButton::toggleLogView()
{
    mDisplayAsLog->setChecked(!mDisplayAsLog->checked());
}

void DataSampleButton::setViewTogglesCallback(std::function<void(bool)> callback) {
    for (int i = 0; i != DataSample::Views::VIEW_COUNT; ++i)
    {
        DataSample::Views view = static_cast<DataSample::Views>(i);
        mViewToggles[view]->setChangeCallback(callback);
    }
}

void DataSampleButton::setDisplayAsLogCallback(std::function<void(bool)> callback)
{
    mDisplayAsLog->setCallback(callback);
}

TEKARI_NAMESPACE_END