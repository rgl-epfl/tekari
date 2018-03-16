#include "bsdf_application.h"

BSDFApplication::BSDFApplication()
:   nanogui::Screen(Eigen::Vector2i(800, 600), "BSDF Visualizer", false)
,   showNormal(true)
,   showLog(false)
,   showSensorPath(false)
,   showPointHeights(false)
{
    using namespace nanogui;

    fileName = "path/to/file";
    imageName = "path/to/image";

    mCanvas = new BSDFCanvas(this);
    mCanvas->setBackgroundColor({100, 100, 100, 255});
    mCanvas->setSize({this->width(), this->height()});

    FormHelper *options = new FormHelper(this);

    ref<Window> openFileWindow = options->addWindow(Eigen::Vector2i(10, 10), "Open file");
    auto fileNameWidget = options->addVariable("File name", fileName);
    fileNameWidget->setSize({200, 20});
    fileNameWidget->setFontSize(20);
    options->addButton("Open", [openFileWindow, this]() mutable {
        mCanvas->openFile(fileName);
        openFileWindow->setVisible(false);
    });
    openFileWindow->center();
    openFileWindow->setVisible(false);

    ref<Window> saveImageWindow = options->addWindow(Eigen::Vector2i(0, 0), "Save image");
    options->addVariable("Image name", imageName);
    options->addButton("Open", [saveImageWindow, this]() mutable {
        std::cout << "Saved image " << fileName << std::endl;
        saveImageWindow->setVisible(false);
    });
    saveImageWindow->center();
    saveImageWindow->setVisible(false);

    ref<Window> optionsWindow = options->addWindow(Eigen::Vector2i(0, 0), "Options");
    options->addGroup("Additional Info");
    options->addVariable("Show normal view", showNormal)->setCallback(
        [this](bool checked) { mCanvas->setShowNormalView(checked); }
    );
    options->addVariable("Show logarithmic view", showLog)->setCallback(
        [this](bool checked) { mCanvas->setShowLogView(checked); }
    );
    options->addVariable("Show sensor path", showSensorPath)->setCallback(
        [this](bool checked) {  mCanvas->setShowPath(checked); }
    );
    options->addVariable("Show point heights", showPointHeights);

    options->addGroup("File");
    options->addButton("Open", [openFileWindow]() mutable {
        openFileWindow->setVisible(true);
        openFileWindow->requestFocus();
    });
    options->addButton("Save image", [saveImageWindow]() mutable {
        saveImageWindow->setVisible(true);
        saveImageWindow->requestFocus();
    });

    setVisible(true);
    performLayout();
}

bool BSDFApplication::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }
    return false;
}

void BSDFApplication::draw(NVGcontext *ctx) {
    /* Draw the user interface */
    Screen::draw(ctx);
}