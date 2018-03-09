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

bool readDataset(   const std::string &filePath,
                    std::vector<nanogui::Vector3f> &data3D,
                    std::vector<nanogui::Vector2f> &data2D);

void createRandomPlane( std::vector<nanogui::Vector3f>& positions3D,
                        std::vector<nanogui::Vector2f> &positions2D,
                        const size_t N);

void triangulatePoints( const std::vector<nanogui::Vector2f> &positions,
                        int dim, const std::string& outputFilePath);

void recoverIndicesFromFile(const std::string &offFilePath,
                            std::vector<unsigned int> &indices);

void computeNormals(const std::vector<nanogui::Vector3f> &positions,
                    const std::vector<unsigned int> &indices,
                    std::vector<nanogui::Vector3f> &normals);


class MyGLCanvas : public nanogui::GLCanvas {
public:
    MyGLCanvas(Widget *parent) : nanogui::GLCanvas(parent) {
        using namespace nanogui;

        mShader.initFromFiles(
            "height_map",
            "../resources/shaders/height_map.vert",
            "../resources/shaders/height_map.frag"
        );

        std::vector<nanogui::Vector3f> positions3D;
        std::vector<nanogui::Vector3f> normals;
        std::vector<nanogui::Vector2f> positions2D;
        std::vector<unsigned int> indices;

        std::string dataSetPath("../resources/golden_paper.txt");
        std::string filePath("../resources/output.txt");
        
        if (!readDataset(dataSetPath, positions3D, positions2D))
        {
            std::cout << "Failed reading file " << dataSetPath << std::endl;
            exit(1);
        }
        
        //createRandomPlane(positions3D, positions2D, 10);
        triangulatePoints(positions2D, 2, filePath);
        recoverIndicesFromFile(filePath, indices);
        //computeNormals(positions3D, indices, normals);

        mFacesCount = indices.size() / 3;

        mShader.bind();

        //mShader.uploadAttrib("in_normal", normals.size(), 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (float*)normals.data());
        mShader.uploadAttrib("in_position", positions3D.size(), 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (float*)positions3D.data());
        mShader.uploadAttrib("indices", mFacesCount, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, indices.data());

        m_arcball.setSize(parent->size());
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
        mShader.setUniform("model", m_arcball.matrix());
        //mShader.setUniform("view", Vector3f(0, 0, 4));

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


struct Bar : std::ostream, std::streambuf
{
    Bar() : std::ostream(this) {}

    int overflow(int c)
    {
        foo(c);
        return 0;
    }


    void foo(char c)
    {
        std::cout.put(c);
        std::cout.put('-');
    }
};

int main(int /* argc */, char ** /* argv */) {
    
    Bar b;
    b << "Hello world !" << std::endl;

    // nanogui
    // try {
    //     nanogui::init();

    //     // scoped variables
    //     {
    //         nanogui::ref<BsdfApplication> app = new BsdfApplication();
    //         app->drawAll();
    //         app->setVisible(true);
    //         nanogui::mainloop();
    //     }

    //     nanogui::shutdown();
    // } catch (const std::runtime_error &e) {
    //     std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
    //     #if defined(_WIN32)
    //         MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
    //     #else
    //         std::cerr << error_msg << endl;
    //     #endif
    //     return -1;
    // }
    return 0;
}

/* Reads a file located at filePath into data.
 * The data shoule be formated line by line, each line conataining "theta phi intensity" info
 */
bool readDataset(const std::string &filePath, std::vector<nanogui::Vector3f> &data3D, std::vector<nanogui::Vector2f> &data2D)
{
    // read file
    float phi, theta, intensity;

    std::ifstream dataset(filePath);
    if (!dataset)
        return false;

    data3D.clear();
    data2D.clear();
    std::string line;

    // min and max values for normalization
    float min_intensity = std::numeric_limits<float>::max();
    float max_intensity = std::numeric_limits<float>::min();
    while (std::getline(dataset, line))
    {
        if (line.size() == 0 || line[0] == '#' || line[0] == '\n') {} /* skip comment/empty lines */
        else {
            std::stringstream ss(line);
            ss >> theta >> phi >> intensity;

            float x = theta * cos(phi * PI / 180.0f) / 90;
            float z = theta * sin(phi * PI / 180.0f) / 90;

            data2D.push_back({x, z});
            data3D.push_back({x, intensity, z});

            min_intensity = std::min(min_intensity, intensity);
            max_intensity = std::max(max_intensity, intensity);
        }
    }

    // intensity normalization
    for(unsigned int i = 1; i < data3D.size(); i += 3)
    {
        data3D[i][1] = (data3D[i][1] - min_intensity) / (max_intensity - min_intensity);
    }

    return true;
}

/* Creates a square plane, with N rows, N columns, and random heights for each point,
 * It stores the result into two arrays, one for the 3d point, and one without height values.
 */
void createRandomPlane(std::vector<nanogui::Vector3f> &positions3D, std::vector<nanogui::Vector2f> &positions2D, const size_t N)
{
    positions3D.resize(N*N);
    positions2D.resize(N*N);

    srand(0);
    for (size_t z = 0; z < N; ++z)
    {
        for (size_t x = 0; x < N; ++x)
        {
            size_t index = z*N + x;
            positions3D[index] = {(float)x/N - 0.5f, (float)(rand() % N) / N / 5, (float)z/N - 0.5f};
            positions2D[index] = {positions3D[index][0], positions3D[index][2]};
        }
    }
}

/* Triangulates the points given in positions, and outputs the result in OFF format in file outputFileStream.
 */
void triangulatePoints(const std::vector<nanogui::Vector2f> &positions, int dim, const std::string& outputFilePath)
{
    std::ofstream outputFile(outputFilePath.c_str());

    const char* input_comment = "testing";

    const char* command = "-d -Qt";
    orgQhull::Qhull qhull;

    qhull.setOutputStream(&outputFile);
    qhull.runQhull(input_comment, dim, positions.size(), (float*)positions.data(), command);
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


void computeNormals(const std::vector<nanogui::Vector3f> &positions,
                    const std::vector<unsigned int> &indices,
                    std::vector<nanogui::Vector3f> &normals)
{
    std::vector<unsigned int> normalPerVertexCount(positions.size(), 0);
    normals.resize(positions.size(), {0, 0, 0});

    for(size_t i = 0; i < indices.size(); i+=3)
    {
        unsigned int i0 = indices[i + 0];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        nanogui::Vector3f faceNormal = (positions[i2]-positions[i0]).cross(positions[i1]-positions[i0]).normalized();

        ++normalPerVertexCount[i0];
        ++normalPerVertexCount[i1];
        ++normalPerVertexCount[i2];
        normals[i0] += faceNormal;
        normals[i1] += faceNormal;
        normals[i2] += faceNormal;
    }

    for(size_t i = 0; i < normals.size(); ++i)
    {
        normals[i] /= normalPerVertexCount[i];
    }
}