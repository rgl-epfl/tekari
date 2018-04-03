#pragma once

#include <nanogui\window.h>
#include <functional>

#include "Metadata.h"

class MetadataWindow : public nanogui::Window
{
public:
	MetadataWindow(nanogui::Widget* parent, const Metadata* metadata, std::function<void(void)> closeCallback);

private:
	std::function<void(void)> m_CloseCallback;
};
