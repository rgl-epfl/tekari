#include "MetadataWindow.h"

#include <nanogui\button.h>

MetadataWindow::MetadataWindow(nanogui::Widget* parent, const DataSample::Metadata & metadata, std::function<void(void)> closeCallback)
	: nanogui::Window(parent, "Metadata")
	, m_Metadata(metadata)
	, m_CloseCallback(closeCallback)
{
	auto closeButton = new nanogui::Button{ buttonPanel(), "", ENTYPO_ICON_CROSS };
	closeButton->setCallback(m_CloseCallback);
}
