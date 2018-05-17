#pragma once

#include <nanogui/common.h>
#include <nanogui/glutil.h>
#include <vector>
#include <memory>
#include <functional>
#include "delaunay.h"
#include "points_stats.h"
#include "Metadata.h"
#include "ColorMap.h"
#include "DataPoint.h"
#include "SelectionBox.h"
#include "Axis.h"
#include "selections.h"

TEKARI_NAMESPACE_BEGIN

#define USES_SHADOWS (1 << 0)
#define DISPLAY_AXIS (1 << 1)
#define DISPLAY_PREDICTED_OUTGOING_ANGLE (1 << 2)

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
                int flags,
                std::shared_ptr<ColorMap> colorMap);

    void setDisplayAsLog(bool displayAsLog);
    inline void toggleView(Views view, bool toggle) { m_DisplayViews[view] = toggle; }
    inline bool displayView(Views view) const { return m_DisplayViews[view]; }

    void linkDataToShaders();
    void initShaders();
    void computeNormalizedHeights();

    // data sample info accessors
    inline const std::string name()                    const { return m_Metadata.sampleName(); }
    inline const Metadata& metadata()                  const { return m_Metadata; }
    inline const PointsStats& pointsInfo()         const { return m_PointsStats; }
    inline bool hasSelection()                         const { return m_SelectionStats.pointsCount() > 0; }
    inline const PointsStats& selectedPointsInfo() const { return m_SelectionStats; }
    inline PointsStats& selectedPointsInfo()             { return m_SelectionStats; }

    inline const std::vector<float>& H()                     const { return m_DisplayAsLog ? m_LH : m_H; }
    inline const std::vector<nanogui::Vector3f>& N()         const { return m_DisplayAsLog ? m_LN : m_N; }
    inline const std::vector<del_point2d_t>& V2D()           const { return m_V2D; }
    inline const std::vector<uint8_t>& selectedPoints()      const { return m_SelectedPoints; }
    inline const std::vector<nanogui::Vector3f>& rawPoints() const { return m_RawPoints; }

    inline std::vector<float>& H()                      { return m_DisplayAsLog ? m_LH : m_H; }
    inline std::vector<nanogui::Vector3f>& N()          { return m_DisplayAsLog ? m_LN : m_N; }
    inline std::vector<del_point2d_t>& V2D()            { return m_V2D; }
    inline std::vector<uint8_t>& selectedPoints()       { return m_SelectedPoints; }
    inline std::vector<nanogui::Vector3f>& rawPoints()  { return m_RawPoints; }

    // Selection
    nanogui::Vector3f selectionCenter();
    void deleteSelectedPoints();
    void updatePointSelection();

    void save(const std::string& path) const;
private:

    void recomputeData();

    // helper methods for data loading/computing
    void readDataset(const std::string &filePath);
    void triangulateData();
    void computePathSegments();
    //void updateSelectionInfo();

    inline static del_point2d_t transformRawPoint(const nanogui::Vector3f& rawPoint)
    {
        return del_point2d_t{ (float)(rawPoint[0] * cos(rawPoint[1] * M_PI / 180.0f) / 90.0f),
            (float)(rawPoint[0] * sin(rawPoint[1] * M_PI / 180.0f) / 90.0f) };
    }

private:
    // Raw sample data
    bool m_ShaderLinked;
    tri_delaunay2d_t*               m_DelaunayTriangulation;
    std::vector<del_point2d_t>      m_V2D;
    std::vector<float>				m_H;
    std::vector<float>              m_LH;
    std::vector<nanogui::Vector3f>  m_N;
    std::vector<nanogui::Vector3f>  m_LN;
    std::vector<unsigned int>       m_PathSegments;
    // Untransformed data
    std::vector<nanogui::Vector3f>  m_RawPoints;        // theta, phi, intensity
    PointsStats                     m_PointsStats;

    // display Shaders
    nanogui::GLShader m_MeshShader;
    nanogui::GLShader m_Shaders[VIEW_COUNT];
    std::function<void(
        const nanogui::Vector3f&,   // view origin
        const nanogui::Matrix4f&,   // model matrix
        const nanogui::Matrix4f&,   // mvp matrix
        bool, std::shared_ptr<ColorMap>)> m_DrawFunctions[VIEW_COUNT];
    nanogui::GLShader m_PredictedOutgoingAngleShader;

    // display options
    bool m_DisplayAsLog;
    bool m_DisplayViews[VIEW_COUNT];

    // metadata
    Metadata m_Metadata;
    Axis m_Axis;

    // Selected point
    std::vector<uint8_t>   m_SelectedPoints;
    PointsStats            m_SelectionStats;
};

TEKARI_NAMESPACE_END