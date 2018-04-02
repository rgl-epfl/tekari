#pragma once

#include <nanogui\window.h>
#include <functional>

#include "DataSample.h"

class MetadataWindow : public nanogui::Window
{
public:
	MetadataWindow(nanogui::Widget* parent, const DataSample::Metadata& metadata, std::function<void(void)> closeCallback);

private:
	const DataSample::Metadata& m_Metadata;
	std::function<void(void)> m_CloseCallback;
};
