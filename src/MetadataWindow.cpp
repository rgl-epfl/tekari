#include "tekari/MetadataWindow.h"

#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/layout.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/label.h>
#include <nanogui/glutil.h>
#include <functional>

using namespace nanogui;

TEKARI_NAMESPACE_BEGIN

MetadataWindow::MetadataWindow(Widget* parent, const Metadata* metadata, std::function<void(void)> closeCallback)
    : Window(parent, "Metadata")
    , mCloseCallback(closeCallback)
{
    auto closeButton = new Button{ buttonPanel(), "", ENTYPO_ICON_CROSS };
    closeButton->setCallback(mCloseCallback);

    setLayout(new GroupLayout{});

	auto scrollContainer = new VScrollPanel{ this };
	scrollContainer->setFixedHeight(300);
	auto container = new Widget{ scrollContainer };
	container->setLayout(new GridLayout{ Orientation::Horizontal, 2, Alignment::Fill, 15, 2 });

	auto rawMeta = metadata->rawMetadata();
	for (const std::string& line : rawMeta) {
		auto pos = line.find_first_of("\t ");
		if (pos == std::string::npos)
			continue;

		std::string title = line.substr(1, pos);
		std::string value = line.substr(pos + 1, line.length());
		if (value.length() > 100) value = value.substr(0, 100);

		new Label(container, title, "sans-bold", 18);
		new Label(container, value);
	}
}

bool MetadataWindow::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Window::keyboardEvent(key, scancode, action, modifiers)) {
        return true;
    }

    if (key == GLFW_KEY_ESCAPE) {
        mCloseCallback();
        return true;
    }

    return false;
}

TEKARI_NAMESPACE_END