#include "MetadataWindow.h"

#include <nanogui\button.h>
#include <nanogui\entypo.h>
#include <nanogui\layout.h>
#include <nanogui\label.h>
#include <functional>

using namespace nanogui;

MetadataWindow::MetadataWindow(Widget* parent, const Metadata* metadata, std::function<void(void)> closeCallback)
	: Window(parent, "Metadata")
	, m_CloseCallback(closeCallback)
{
	auto closeButton = new Button{ buttonPanel(), "", ENTYPO_ICON_CROSS };
	closeButton->setCallback(m_CloseCallback);

	setLayout(new GroupLayout{});
	setFixedWidth(650);

	auto infos = new Widget{this};
	infos->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Fill });

	auto addInfoSection = [&infos](const std::string& label) {
		new Label{ infos, label, "sans-bold", 18 };
		auto section = new Widget{ infos };
		section->setLayout(new BoxLayout{ Orientation::Vertical, Alignment::Fill, 0, 0 });
		return section;
	};

	auto addSpacer = [&infos](unsigned int h) {
		auto spacer = new Widget{infos};
		spacer->setHeight(h);
	};

	auto addInfo = [this](Widget* section, const std::string& desc, const std::string& info) {
		auto row = new Widget{ section };
		row->setLayout(new BoxLayout{ Orientation::Horizontal, Alignment::Fill});
		auto indent = new Widget{ row };
		indent->setFixedWidth(10);
		auto descLabel = new Label{ row, desc + " : " };
		descLabel->setFixedWidth(200);
		new Label{row, info};
	};

	auto generalSection = addInfoSection("General");
	addInfo(generalSection, "Mountain Version", metadata->mountainVersion);
	addInfo(generalSection, "Measured At", metadata->measuredAt);
	addInfo(generalSection, "Read From Database At", metadata->dataReadFromDatabaseAt);
	addSpacer(10);

	auto sampleInfo = addInfoSection("Data Sample");
	addInfo(sampleInfo, "Sample Label", metadata->sampleLabel);
	addInfo(sampleInfo, "Sample Name", metadata->sampleName);
	addInfo(sampleInfo, "Datapoints In Sample", std::to_string(metadata->datapointsInFile));
	addSpacer(10);

	auto techInfo = addInfoSection("Technical Details");
	addInfo(techInfo, "Lamp Used", metadata->lamp);
	addInfo(techInfo, "Incident Theta", std::to_string(metadata->incidentTheta));
	addInfo(techInfo, "Incident Phi", std::to_string(metadata->incidentPhi));
	addSpacer(10);

	auto dbSection = addInfoSection("Database");
	addInfo(dbSection, "Database Host", metadata->databaseHost);
	addInfo(dbSection, "Database Name", metadata->databaseName);
	addInfo(dbSection, "Database Id", std::to_string(metadata->databaseId));
	addInfo(dbSection, "Datapoints In Database", std::to_string(metadata->datapointsInDatabase));
	addInfo(techInfo, "Front Integral", std::to_string(metadata->frontIntegral));
}
