#pragma once

#include <nanogui/nanogui.h>
#include <vector>

#include "sample_data_parser.h"

struct SampleData
{
private:
    nanogui::GLShader m_NormalShader;
    nanogui::GLShader m_LogShader;
    nanogui::GLShader m_PathShader;
    unsigned int m_NFaces;

    bool m_DisplayNormalView;
    bool m_DisplayPath;
    bool m_DisplayLogView;

public:
    SampleData(const std::string& sampleDataPath);
    ~SampleData();

    void draw(  const nanogui::Matrix4f& model,
                const nanogui::Matrix4f& view,
                const nanogui::Matrix4f& proj);

    void displayNormalView(bool value) { m_DisplayNormalView = value; }
    void displayLogView(bool value) { m_DisplayLogView = value; }
    void displayPath(bool value) { m_DisplayPath = value; }

    void loadFromFile(const std::string& sampleDataPath);
};