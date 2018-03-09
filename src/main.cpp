#include <libqhullcpp/Qhull.h>
#include <libqhullcpp/QhullVertexSet.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>

#define PI 3.14159

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
#include <cstdlib>
#include <memory>
#include <utility>

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

bool readDataset(const std::string &filePath, std::vector<double> &data3D, std::vector<double> &data2D);
void createRandomPlane(std::vector<double>& positions3D, std::vector<double> &positions2D, const size_t N);
void triangulatePoints(const std::vector<double> &positions, int dim, int count, const std::string& outputFilePath);
void recoverIndicesFromFile(const std::string &offFilePath, std::vector<unsigned int> &indices);


class MyGLCanvas : public nanogui::GLCanvas {
public:
    MyGLCanvas(Widget *parent) : nanogui::GLCanvas(parent) {
        using namespace nanogui;

        mShader.init(
            /* An identifying name */
            "a_simple_shader",

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 modelViewProj;\n"
            "in vec3 position;\n"
            "out float height;\n"
            "void main() {\n"
            "    gl_Position = modelViewProj * vec4(position, 1.0);\n"
            "    height = position.y;"
            "}",

            /* Fragment shader */
            "#version 330\n"
            "in float height;\n"
            "out vec4 color;\n"
            "void main() {\n"
            "    color = vec4(height, 1 - abs((height - 0.5)*2), 1-height, 1.0f);\n"
            "}"
        );

        std::vector<double> positions3D;
        std::vector<double> positions2D;

        std::string dataSetPath("../resources/golden_paper.txt");
        std::string filePath("../resources/output.txt");
        std::vector<unsigned int> indices;

        if (!readDataset(dataSetPath, positions3D, positions2D))
        {
            std::cout << "Failed reading file " << dataSetPath << std::endl;
            exit(1);
        }
        triangulatePoints(positions2D, 2, positions2D.size()/2, filePath);
        recoverIndicesFromFile(filePath, indices);

        mFacesCount = indices.size() / 3;

        mShader.bind();

        mShader.uploadAttrib("position", positions3D.size() / 3, 3, 3 * sizeof(double), GL_DOUBLE, GL_FALSE, positions3D.data());
        mShader.uploadAttrib("indices", mFacesCount, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, indices.data());

        m_arcball.setSize(mSize);
    }

    ~MyGLCanvas() {
        mShader.free();
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

        mShader.bind();

        Matrix4f view, proj;
        view = lookAt(Vector3f(0, 0, 4), Vector3f(0, 0, 0), Vector3f(0, 1, 0));
        const float viewAngle = 30, near = 0.01, far = 100;
        float fH = std::tan(viewAngle / 360.0f * M_PI) * near;
        float fW = fH * (float) mSize.x() / (float) mSize.y();
        proj = frustum(-fW, fW, -fH, fH, near, far);

        Matrix4f mvp = proj * view * m_arcball.matrix();
        mShader.setUniform("modelViewProj", mvp);

        glEnable(GL_DEPTH_TEST);
        glPointSize(4);
        /* Draw 12 triangles starting at index 0 */
        mShader.drawIndexed(GL_TRIANGLES, 0, mFacesCount);
        glDisable(GL_DEPTH_TEST);
    }

private:
    nanogui::GLShader mShader;
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

    // // read file
    // std::vector<double> data;
    // readDataset("../resources/golden_paper.txt", data);

    // std::cout << std::setprecision(10);
    // for(int i = 0; i < 10; ++i)
    // {
    //     std::cout << "x : " << data[3*i+0] << std::endl;
    //     std::cout << "y : " << data[3*i+1] << std::endl;
    //     std::cout << "h : " << data[3*i+2] << std::endl;
    // }

    // test qhull

    // std::ofstream outputFile("../resources/output.txt");

    // const char* input_comment = "lol";
    // int dim = 3;
    // int count = 5;
    // realT coords[] =
    // {
    //     0, 0, 1,
    //     1, 1, 0,
    //     0, 1, 3,
    //     1, 0, 0.5,
    //     2, 1, -1.2
    // };
    // const char* command = "-d -Qt";
    // orgQhull::Qhull qhull;

    // qhull.setOutputStream(&outputFile);
    // qhull.runQhull(input_comment, dim, count, coords, command);
    // qhull.outputQhull("-o");

    return 0;
}

/* Reads a file located at filePath into data.
 * The data shoule be formated line by line, each line conataining "theta phi intensity" info
 */
bool readDataset(const std::string &filePath, std::vector<double> &data3D, std::vector<double> &data2D)
{
    // read file
    double phi, theta, intensity;

    std::ifstream dataset(filePath);
    if (!dataset)
        return false;

    data3D.clear();
    std::string line;

    // min and max values for normalization
    double min_intensity = std::numeric_limits<double>::max();
    double max_intensity = std::numeric_limits<double>::min();
    while (std::getline(dataset, line))
    {
        if (line.size() == 0 || line[0] == '#' || line[0] == '\n') {} /* skip comment/empty lines */
        else {
            std::stringstream ss(line);
            ss >> theta >> phi >> intensity;

            double x = theta * cos(phi * PI / 180.0f) / 90;
            double z = theta * sin(phi * PI / 180.0f) / 90;

            data2D.push_back(x);
            data2D.push_back(z);

            data3D.push_back(x);
            data3D.push_back(intensity);
            data3D.push_back(z);

            min_intensity = std::min(min_intensity, intensity);
            max_intensity = std::max(max_intensity, intensity);
        }
    }

    // intensity normalization
    for(unsigned int i = 1; i < data3D.size(); i += 3)
    {
        data3D[i] = (data3D[i] - min_intensity) / (max_intensity - min_intensity);
    }

    return true;
}

/* Creates a square plane, with N rows, N columns, and random heights for each point,
 * It stores the result into two arrays, one for the 3d point, and one without height values.
 */
void createRandomPlane(std::vector<double> &positions3D, std::vector<double> &positions2D, const size_t N)
{
    srand(0);
    for (size_t z = 0; z < N; ++z)
    {
        for (size_t x = 0; x < N; ++x)
        {
            size_t index = z*N + x;
            positions3D[3*index + 0] = (double)x/N - 0.5;
            positions3D[3*index + 1] = (double)(rand() % N) / N;
            positions3D[3*index + 2] = (double)z/N - 0.5;

            positions2D[2*index + 0] = positions3D[3*index + 0];
            positions2D[2*index + 1] = positions3D[3*index + 2];
        }
    }
}

/* Triangulates the points given in positions, and outputs the result in OFF format in file outputFileStream.
 */
void triangulatePoints(const std::vector<double> &positions, int dim, int count, const std::string& outputFilePath)
{
    std::ofstream outputFile(outputFilePath.c_str());

    const char* input_comment = "testing";

    const char* command = "-d -Qt";
    orgQhull::Qhull qhull;

    qhull.setOutputStream(&outputFile);
    qhull.runQhull(input_comment, dim, count, positions.data(), command);
    qhull.outputQhull("-o");
}

/* Recovers the indices stroed in OFF format in the given file.
 */
void recoverIndicesFromFile(const std::string &offFilePath, std::vector<unsigned int> &indices)
{
    std::ifstream offFile(offFilePath.c_str());
    std::string line;
    // remove first line
    getline(offFile, line);

    unsigned int nVertices, v0, v1, v2;
    while (offFile.is_open() && !offFile.eof() && !offFile.fail())
    {
        getline(offFile, line);
        if (line[0] == '3') { // face line
            std::stringstream ss(line);
            ss >> nVertices >> v0 >> v1 >> v2;
            indices.push_back(v0);
            indices.push_back(v1);
            indices.push_back(v2);
        }
    }
}