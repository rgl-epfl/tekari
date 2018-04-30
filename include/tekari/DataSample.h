#pragma once

#include <nanogui/common.h>
#include <nanogui/glutil.h>
#include <vector>
#include <memory>
#include <functional>
#include "delaunay.h"
#include "PointSampleInfo.h"
#include "Metadata.h"
#include "ColorMap.h"
#include "DataPoint.h"
#include "BitMap.h"
#include "SelectionBox.h"
#include "Axis.h"

TEKARI_NAMESPACE_BEGIN

class DataSample
{
public:
    // Usefull types
    enum Views
    {
        PATH = 0,
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
    // constructors/destructors, assignement operators
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

    void setDisplayAsLog(bool displayAsLog);
    void toggleView(Views view, bool toggle) { m_DisplayViews[view] = toggle; }
    bool displayView(Views view) const { return m_DisplayViews[view]; }

    void linkDataToShaders();
    void initShaders();

    // data sample info accessors
    //const std::string& name()                   const { return "hello there"; }// m_Metadata.sampleName; }
    const PointSampleInfo& pointsInfo()         const { return m_PointsInfo; }
    const PointSampleInfo& selectedPointsInfo() const { return m_SelectedPointsInfo; }

    const Metadata& metadata() const { return m_Metadata; }

    // Selection
    void selectPoints(const nanogui::Matrix4f& mvp,
        const SelectionBox& selectionBox,
        const nanogui::Vector2i & canvasSize,
        SelectionMode mode);
    void deselectAllPoints();
    void selectHighestPoint();
    nanogui::Vector3f selectionCenter();
    bool deleteSelectedPoints();

    void save(const std::string& path) const;
private:

    // helper methods for data loading/computing
    void readDataset(const std::string &filePath);
    void triangulateData();
    void computePathSegments();
    inline nanogui::Vector3f getVertex(unsigned int i, bool logged) const;
    void computeTriangleNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged);
    void computeNormals();

    static del_point2d_t transformRawPoint(const nanogui::Vector3f& rawPoint)
    {
        return del_point2d_t{ (float)(rawPoint[0] * cos(rawPoint[1] * M_PI / 180.0f) / 90.0f),
                                (float)(rawPoint[0] * sin(rawPoint[1] * M_PI / 180.0f) / 90.0f) };
    }

private:
    // Raw sample data
    bool m_ShaderLinked;
    tri_delaunay2d_t *tri_delaunay2d;
    std::vector<del_point2d_t>      m_2DPoints;
    std::vector<float>				m_Heights;
    std::vector<float>              m_LogHeights;
    std::vector<nanogui::Vector3f>  m_Normals;
    std::vector<nanogui::Vector3f>  m_LogNormals;
    std::vector<unsigned int>       m_PathSegments;
    // Untransformed data
    std::vector<nanogui::Vector3f>  m_RawPoints;        // theta, phi, intensity
    PointSampleInfo                 m_PointsInfo;

    // display Shaders
    nanogui::GLShader m_MeshShader;
    nanogui::GLShader m_Shaders[VIEW_COUNT];
    std::function<void(
        const nanogui::Vector3f&,   // view origin
        const nanogui::Matrix4f&,   // model matrix
        const nanogui::Matrix4f&,   // mvp matrix
        bool, std::shared_ptr<ColorMap>)> m_DrawFunctions[VIEW_COUNT];

    // display options
    bool m_DisplayAsLog;
    bool m_DisplayViews[VIEW_COUNT];

    // metadata
    Metadata m_Metadata;
    std::string m_RawMetaData;
    Axis m_Axis;

    // Selected point
    std::vector<unsigned char>   m_SelectedPoints;
    //BitMap              m_SelectedPoints;
    PointSampleInfo     m_SelectedPointsInfo;
};

TEKARI_NAMESPACE_END