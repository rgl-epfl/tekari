#pragma once

#include <nanogui\widget.h>
#include <nanogui/opengl.h>
#include <functional>

class DataSampleButton : public nanogui::Widget
{
public:
	DataSampleButton(nanogui::Widget* parent, const std::string &label);

	virtual bool mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers) override;
	virtual void draw(NVGcontext *ctx) override;

	virtual nanogui::Vector2i preferredSize(NVGcontext *ctx) const override {
		nvgFontSize(ctx, mFontSize);
		nvgFontFace(ctx, "sans-bold");
		float labelSize = nvgTextBounds(ctx, 0, 0, m_Label.c_str(), nullptr, nullptr);

		return nanogui::Vector2i(static_cast<int>(labelSize) + 15, mFontSize + 6);
	}

	void toggleView();

	bool isSelected() const { return m_IsSelected; }
	void setIsSelected(bool isSelected) { m_IsSelected = isSelected; }

	void setCallback(std::function<void(DataSampleButton*)> callback) { m_Callback = callback; }
	void setDeleteCallback(std::function<void(DataSampleButton*)> callback) { m_DeleteCallback = callback; }
	void setToggleViewCallback(std::function<void(bool, DataSampleButton*)> callback) { m_ToggleViewCallback = callback; }

private:
	std::string m_Label;
	bool m_IsSelected;
	bool m_IsVisible;

	nanogui::ToolButton* m_ToogleViewButton;

	std::function<void(DataSampleButton*)> m_Callback;
	std::function<void(bool, DataSampleButton*)> m_ToggleViewCallback;
	std::function<void(DataSampleButton*)> m_DeleteCallback;
};