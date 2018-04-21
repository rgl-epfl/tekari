#pragma once

#include <nanogui/common.h>
#include <nanogui/glutil.h>
#include <vector>
#include <memory>
#include <functional>
#include "delaunay.h"
#include "Metadata.h"
#include "ColorMap.h"
#include "DataPoint.h"
#include "Axis.h"

struct DataSample
{
    enum Views
    {
        NORMAL = 0,
        LOG,
        PATH,
        POINTS,
        INCIDENT_ANGLE,
        VIEW_COUNT
    };

    enum SelectionMode
    {
        STANDARD,
        ADD,
        SUBTRACT
    };

    struct PointSampleInfo
    {
        unsigned int pointCount;
        std::pair<float, float> minMaxHeights;
        nanogui::Vector3f averagePoint;

        PointSampleInfo()
        :   pointCount(0)
        ,   minMaxHeights(std::make_pair<float, float>(0.0f, 0.0f))
        ,   averagePoint{0.0f, 0.0f, 0.0f}
        {}
    };

public:
    DataSample();
    DataSample(const std::string& sampleDataPath);
    DataSample(const DataSample&) = delete;
    DataSample(DataSample&&) = default;

    ~DataSample();

    DataSample& operator=(const DataSample&) = delete;
    DataSample& operator=(DataSample&&) = default;

    void drawGL(const nanogui::Vector3f& viewOrigin,
                const nanogui::Matrix4f& model,
                const nanogui::Matrix4f& view,
                const nanogui::Matrix4f& proj,
                bool useShadows, bool displayAxis,
                std::shared_ptr<ColorMap> colorMap);

    void toggleView(Views view, bool toggle) { m_DisplayViews[view] = toggle; }
    bool displayView(Views view) const { return m_DisplayViews[view]; }

    void loadFromFile(const std::string& sampleDataPath);
    void linkDataToShaders();

    std::string name()          const { return m_Metadata.sampleName; }
    unsigned int pointsCount()  const { return tri_delaunay2d->num_points; }
    float minHeight()           const { return m_PointsInfo.minMaxHeights.first; }
    float maxHeight()           const { return m_PointsInfo.minMaxHeights.second; }
    float averageHeight()       const { return m_PointsInfo.averagePoint[1]; }

    const Metadata& metadata() const { return m_Metadata; }

    void selectPoints(const nanogui::Matrix4f& mvp,
        const nanogui::Vector2i& topLeft,
        const nanogui::Vector2i& size,
        const nanogui::Vector2i & canvasSize,
        SelectionMode mode);
    void deselectAllPoints();
    nanogui::Vector3f selectionCenter();

private:
    void readDataset(const std::string &filePath, std::vector<del_point2d_t> &points);
    inline nanogui::Vector3f getVertex(unsigned int i, bool logged) const;
    void computeTriangleNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged);
    void computeNormals();

private:
    // Raw sample data
    bool m_ShaderLinked;
    tri_delaunay2d_t *tri_delaunay2d;
    std::vector<float>				m_Heights;
    std::vector<float>              m_LogHeights;
    std::vector<nanogui::Vector3f>  m_Normals;
    std::vector<nanogui::Vector3f>  m_LogNormals;
    // Untransformed data
    std::vector<RawDataPoint>           m_RawPoints;
    PointSampleInfo                     m_PointsInfo;

    // display Shaders
    nanogui::GLShader m_Shaders[VIEW_COUNT];
    std::function<void(const nanogui::Vector3f&, const nanogui::Matrix4f&, const nanogui::Matrix4f&,
        bool, std::shared_ptr<ColorMap>)> m_DrawFunctions[VIEW_COUNT];
    std::vector<unsigned int> m_PathSegments;

    // display options
    bool m_DisplayViews[VIEW_COUNT];

    // metadata
    Metadata m_Metadata;
    Axis m_Axis;

    // Selected point
    std::vector<char>   m_SelectedPoints;
    PointSampleInfo     m_SelectedPointsInfo;
};