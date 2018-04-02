#include "DataSampleButton.h"

#include <iostream>

#include <nanogui/opengl.h>
#include <nanogui/Layout.h>
#include <nanogui/Label.h>
#include <nanogui/Button.h>
#include <nanogui/ToolButton.h>
#include <nanogui/entypo.h>

using namespace nanogui;

DataSampleButton::DataSampleButton(Widget * parent, const std::string & label)
:	Widget(parent)
,	m_Label(label)
,	m_IsSelected(false)
{
	setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5, 5));
	std::string fixedSizeLabel = label.size() > 20 ? label.substr(0, 17) + "..." : label;
	new Label(this, fixedSizeLabel, "sans", 20);
	
	m_ToogleViewButton = new ToolButton(this, ENTYPO_ICON_EYE);
	m_ToogleViewButton->setChangeCallback([this](bool checked) { m_ToggleViewCallback(checked, this); });
	m_ToogleViewButton->setPushed(true);
	m_ToogleViewButton->setTooltip("Toogle visibility of this data sample (ENTER)");
	
	auto deleteButton = new ToolButton(this, ENTYPO_ICON_CROSS);
	deleteButton->setCallback([this]() { m_DeleteCallback(this); });
	deleteButton->setTooltip("Deletes this data sample (DELETE)");
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
		if (m_Callback) {
			m_Callback(this);
		}
		return true;
	}

	return false;
}

void DataSampleButton::draw(NVGcontext * ctx)
{
	// Fill the button with color.
	if (m_IsSelected || mMouseFocus) {
		nvgBeginPath(ctx);

		nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());

		nvgFillColor(ctx, m_IsSelected ? Color(1.0f, 0.3f) : Color(1.0f, 0.1f));
		nvgFill(ctx);
	}
	Widget::draw(ctx);
}

void DataSampleButton::toggleView()
{
	m_ToogleViewButton->setPushed(!m_ToogleViewButton->pushed());
	m_ToogleViewButton->changeCallback()(m_ToogleViewButton->pushed());
}
