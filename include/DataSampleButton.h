#pragma once

#include <nanogui\widget.h>
#include <nanogui/opengl.h>
#include <functional>

class DataSampleButton : public nanogui::Widget
{
public:
	DataSampleButton(nanogui::Widget* parent, const std::string &label, int id);

	virtual bool mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers) override;
	virtual void draw(NVGcontext *ctx) override;

	virtual nanogui::Vector2i preferredSize(NVGcontext *ctx) const override {
		nvgFontSize(ctx, mFontSize);
		nvgFontFace(ctx, "sans-bold");
		float labelSize = nvgTextBounds(ctx, 0, 0, m_Label.c_str(), nullptr, nullptr);

		return nanogui::Vector2i(static_cast<int>(labelSize) + 15, mFontSize + 6);
	}

	bool isSelected() const { return m_IsSelected; }
	void setIsSelected(bool isSelected) { m_IsSelected = isSelected; }

	int id() const { return m_Id; }
	void setId(int id) { m_Id = id; }

	void setSelectedCallback(std::function<void(int)> callback) { m_SelectedCallback = callback; }
	void setDeleteCallback(std::function<void(int)> callback) { m_DeleteCallback = callback; }

private:

	std::string m_Label;
	int m_Id;
	bool m_IsSelected;
	bool m_IsVisible;
	std::function<void(int)> m_SelectedCallback;
	std::function<void(int)> m_DeleteCallback;
};