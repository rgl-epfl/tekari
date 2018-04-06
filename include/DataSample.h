#pragma once

#include <nanogui/common.h>
#include <nanogui/glutil.h>
#include <vector>
#include <memory>
#include "delaunay.h"
#include "Metadata.h"
#include "ColorMap.h"

struct DataSample
{
    enum Views
    {
        NORMAL = 0,
        LOG,
        PATH,
        VIEW_COUNT
    };

    enum SelectionMode
    {
        STANDARD,
        ADD,
        SUBTRACT
    };

public:
    DataSample(std::shared_ptr<ColorMap> colorMap);
    DataSample(std::shared_ptr<ColorMap> colorMap, const std::string& sampleDataPath);
    DataSample(const DataSample&) = delete;
    DataSample(DataSample&&) = default;

    ~DataSample();

    DataSample& operator=(const DataSample&) = delete;
    DataSample& operator=(DataSample&&) = default;

    void drawGL(const nanogui::Vector3f& viewOrigin,
                const nanogui::Matrix4f& model,
                const nanogui::Matrix4f& view,
                const nanogui::Matrix4f& proj);

    void toggleView(Views view) { m_DisplayViews[view] = !m_DisplayViews[view]; }
    bool displayView(Views view) const { return m_DisplayViews[view]; }

    void setColorMap(std::shared_ptr<ColorMap> colorMap) { m_ColorMap = colorMap; }

    void loadFromFile(const std::string& sampleDataPath);

    float minHeight() const { return m_MinMaxHeights.first; }
    float maxHeight() const { return m_MinMaxHeights.second; }

    const Metadata& metadata() const { return m_Metadata; }

    void selectPoints(const nanogui::Matrix4f& mvp,
        const nanogui::Vector2i& topLeft,
        const nanogui::Vector2i& size,
        const nanogui::Vector2i & canvasSize,
        SelectionMode mode);

private:
    inline nanogui::Vector3f getVertex(unsigned int i, bool logged) const;
    void computeTriangleNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged);

    void readDataset(const std::string &filePath, std::vector<del_point2d_t> &points);
    void computeNormals();
    void linkDataToShaders();

private:
    // Raw sample data
    tri_delaunay2d_t *tri_delaunay2d;
    std::vector<float>				m_Heights;
    std::vector<float>              m_LogHeights;
    std::vector<nanogui::Vector3f>  m_Normals;
    std::vector<nanogui::Vector3f>  m_LogNormals;
    std::pair<float, float>         m_MinMaxHeights;

    // display Shaders
    nanogui::GLShader m_NormalShader;
    nanogui::GLShader m_LogShader;
    nanogui::GLShader m_PathShader;
    std::shared_ptr<ColorMap> m_ColorMap;
    std::vector<unsigned int> m_PathSegments;

    // display options
    bool m_DisplayViews[VIEW_COUNT];

    // metadata
    Metadata m_Metadata;

    // Selected point
    nanogui::GLShader m_SelectedPointsShader;
    std::vector<char> m_SelectedPoints;
};