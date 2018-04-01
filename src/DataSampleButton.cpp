#include "DataSampleButton.h"

#include <nanogui/opengl.h>
#include <nanogui/Layout.h>
#include <nanogui/Label.h>
#include <nanogui/Button.h>
#include <nanogui/entypo.h>

using namespace nanogui;

DataSampleButton::DataSampleButton(Widget * parent, const std::string & label, int id)
:	Widget(parent)
,	m_Label(label)
,	m_Id(id)
,	m_IsSelected(false)
{
	setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill, 5, 5));
	new Label(this, label, "sans", 20);
	
	auto deleteButton = new Button(this, "", ENTYPO_ICON_CROSS);
	deleteButton->setCallback([this]() { m_DeleteCallback(m_Id); });
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
		if (!m_IsSelected) {
			// Unselect the other, currently selected image.
			for (auto widget : parent()->children()) {
				DataSampleButton* b = dynamic_cast<DataSampleButton*>(widget);
				if (b && b != this) {
					b->m_IsSelected = false;
				}
			}

			m_IsSelected = true;
			if (m_SelectedCallback) {
				m_SelectedCallback(m_Id);
			}
		}
		return true;
	}

	return false;
}

void DataSampleButton::draw(NVGcontext * ctx)
{
	Widget::draw(ctx);

	// Fill the button with color.
	if (m_IsSelected || mMouseFocus) {
		nvgBeginPath(ctx);

		nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());

		nvgFillColor(ctx, m_IsSelected ? Color(1.0f, 0.5f) : Color(1.0f, 0.1f));
		nvgFill(ctx);
	}
}
