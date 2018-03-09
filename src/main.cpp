#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

/*
    src/example4.cpp -- C++ version of an example application that shows
    how to use the OpenGL widget. For a Python implementation, see
    '../python/example4.py'.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/nanogui.h>

// Includes for the GLTexture class.
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include "sample_data.h"

#if defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#if defined(_WIN32)
#  pragma warning(push)
#  pragma warning(disable: 4457 4456 4005 4312)
#endif

#if defined(_WIN32)
#  pragma warning(pop)
#endif
#if defined(_WIN32)
#  if defined(APIENTRY)
#    undef APIENTRY
#  endif
#  include <windows.h>
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::pair;
using std::to_string;

void createRandomPlane( SampleDataParser &os,
                        const size_t N);

class MyGLCanvas : public nanogui::GLCanvas {
public:
    MyGLCanvas(Widget *parent)
    :   nanogui::GLCanvas(parent)
    ,   m_SampleData("../resources/golden_paper.txt")
    {
        using namespace nanogui;

// TODO: Change this
        m_arcball.setSize(parent->size());
    }

    bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override {
        m_arcball.motion(p);
        return true;
    }

    bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
        if (button == GLFW_MOUSE_BUTTON_1) {
            m_arcball.button(p, down);
            return true;
        }
        return false;
    }

    virtual void drawGL() override {
        using namespace nanogui;


        Matrix4f view, proj;
        view = lookAt(Vector3f(0, 0, 4),
                      Vector3f(0, 0, 0),
                      Vector3f(0, 1, 0));
        const float viewAngle = 30, near = 0.01, far = 100;
        float fH = std::tan(viewAngle / 360.0f * M_PI) * near;
        float fW = fH * (float) mSize.x() / (float) mSize.y();
        proj = frustum(-fW, fW, -fH, fH, near, far);

        m_SampleData.draw(m_arcball.matrix(), view, proj);
    }

private:
    SampleData m_SampleData;
    unsigned int mFacesCount;

    nanogui::Arcball m_arcball;
};


class BsdfApplication : public nanogui::Screen {
public:
    BsdfApplication() : nanogui::Screen(Eigen::Vector2i(800, 600), "BSDF Visualizer", false) {
        using namespace nanogui;

        fileName = "path/to/file";
        imageName = "path/to/image";

        mCanvas = new MyGLCanvas(this);
        mCanvas->setBackgroundColor({100, 100, 100, 255});
        mCanvas->setSize({this->width(), this->height()});

        FormHelper *options = new FormHelper(this);

        ref<Window> openFileWindow = options->addWindow(Eigen::Vector2i(10, 10), "Open file");
        options->addVariable("File name", fileName);
        options->addButton("Open", [openFileWindow, this]() mutable {
            std::cout << "Opened file " << fileName << std::endl;
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
        options->addVariable("Show logarithmic view", showLog);
        options->addVariable("Show sensor path", showSensorPath);
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

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
        if (Screen::keyboardEvent(key, scancode, action, modifiers))
            return true;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            setVisible(false);
            return true;
        }
        return false;
    }

    virtual void draw(NVGcontext *ctx) {
        /* Draw the user interface */
        Screen::draw(ctx);
    }
private:
    MyGLCanvas *mCanvas;

    bool showLog;
    bool showSensorPath;
    bool showPointHeights;

    std::string fileName;
    std::string imageName;
};

int main(int /* argc */, char ** /* argv */) {
    

    // nanogui
    try {
        nanogui::init();

        // scoped variables
        {
            nanogui::ref<BsdfApplication> app = new BsdfApplication();
            app->drawAll();
            app->setVisible(true);
            nanogui::mainloop();
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            std::cerr << error_msg << endl;
        #endif
        return -1;
    }
    return 0;
}
