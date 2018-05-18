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
#include "Axis.h"

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
    inline void toggleView(Views view, bool toggle) { mDisplayViews[view] = toggle; }
    inline bool displayView(Views view) const { return mDisplayViews[view]; }

    void linkDataToShaders();
    void initShaders();

    // info accessors
    inline const std::string name()             const { return mMetadata.sampleName(); }
    inline const Metadata& metadata()           const { return mMetadata; }
    inline bool hasSelection()                  const { return mSelectionStats.pointsCount() > 0; }
    inline const PointsStats& pointsStats()     const { return mPointsStats; }
    inline const PointsStats& selectionStats()  const { return mSelectionStats; }
    inline PointsStats& pointsStats()                 { return mPointsStats; }
    inline PointsStats& selectionStats()              { return mSelectionStats; }

    // accessors
    inline const VectorXf& currH()                           const { return mDisplayAsLog ? mLH : mH; }
    inline const std::vector<nanogui::Vector3f>& currN()     const { return mDisplayAsLog ? mLN : mN; }
    inline const std::vector<del_point2d_t>& V2D()           const { return mV2D; }
    inline const std::vector<uint8_t>& selectedPoints()      const { return mSelectedPoints; }
    inline const std::vector<nanogui::Vector3f>& rawPoints() const { return mRawPoints; }

    inline VectorXf& currH()                            { return mDisplayAsLog ? mLH : mH; }
    inline VectorXf& H()                                { return mH; }
    inline VectorXf& LH()                               { return mLH; }
    inline std::vector<nanogui::Vector3f>& currN()      { return mDisplayAsLog ? mLN : mN; }
    inline std::vector<nanogui::Vector3f>& N()          { return mN; }
    inline std::vector<nanogui::Vector3f>& LN()         { return mLN; }
    inline std::vector<del_point2d_t>& V2D()            { return mV2D; }
    inline std::vector<uint8_t>& selectedPoints()       { return mSelectedPoints; }
    inline std::vector<nanogui::Vector3f>& rawPoints()  { return mRawPoints; }
    inline std::vector<unsigned int>& pathSegments()    { return mPathSegments; }

    inline tri_delaunay2d_t** triangulation()           { return &mDelaunayTriangulation; }

    // Selection
    void centerAxisToSelection();
    nanogui::Vector3f selectionCenter() const;
    void updatePointSelection();

    void save(const std::string& path) const;
private:

    // helper methods for data loading/computing
    void readDataset(const std::string &filePath);

    inline static del_point2d_t transformRawPoint(const nanogui::Vector3f& rawPoint)
    {
        return del_point2d_t{ (float)(rawPoint[0] * cos(rawPoint[1] * M_PI / 180.0f) / 90.0f),
            (float)(rawPoint[0] * sin(rawPoint[1] * M_PI / 180.0f) / 90.0f) };
    }

private:
    // Raw sample data
    tri_delaunay2d_t*               mDelaunayTriangulation;
    std::vector<del_point2d_t>      mV2D;
    VectorXf				        mH;
    VectorXf              mLH;
    std::vector<nanogui::Vector3f>  mN;
    std::vector<nanogui::Vector3f>  mLN;
    std::vector<unsigned int>       mPathSegments;
    // Untransformed data
    std::vector<nanogui::Vector3f>  mRawPoints;        // theta, phi, intensity
    PointsStats                     mPointsStats;

    // display Shaders
    nanogui::GLShader mMeshShader;
    nanogui::GLShader mShaders[VIEW_COUNT];
    std::function<void(
        const nanogui::Vector3f&,   // view origin
        const nanogui::Matrix4f&,   // model matrix
        const nanogui::Matrix4f&,   // mvp matrix
        bool, std::shared_ptr<ColorMap>)> mDrawFunctions[VIEW_COUNT];
    nanogui::GLShader mPredictedOutgoingAngleShader;

    // display options
    bool mDisplayAsLog;
    bool mDisplayViews[VIEW_COUNT];

    // metadata
    Metadata mMetadata;
    Axis mAxis;

    // Selected point
    std::vector<uint8_t>   mSelectedPoints;
    PointsStats            mSelectionStats;
};

TEKARI_NAMESPACE_END