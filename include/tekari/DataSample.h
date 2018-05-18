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
    DataSample();
    DataSample(const DataSample&) = delete;
    DataSample(DataSample&&) = default;
    ~DataSample();
    DataSample& operator=(const DataSample&) = delete;
    DataSample& operator=(DataSample&&) = default;

    void drawGL(const Vector3f& viewOrigin,
                const Matrix4f& model,
                const Matrix4f& view,
                const Matrix4f& proj,
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

    inline PointsStats& pointsStats()   { return mPointsStats; }
    inline Metadata& metadata()         { return mMetadata; }
    inline PointsStats& selectionStats(){ return mSelectionStats; }

    // accessors
    inline const VectorXf& currH()           const { return mDisplayAsLog ? mLH : mH; }
    inline const MatrixXf& currN()           const { return mDisplayAsLog ? mLN : mN; }
    inline const MatrixXf& V2D()             const { return mV2D; }
    inline const MatrixXf& rawPoints()       const { return mRawPoints; }
    inline const VectorXu8& selectedPoints() const { return mSelectedPoints; }

    inline VectorXf& currH()            { return mDisplayAsLog ? mLH : mH; }
    inline VectorXf& H()                { return mH; }
    inline VectorXf& LH()               { return mLH; }
    inline MatrixXf& currN()            { return mDisplayAsLog ? mLN : mN; }
    inline MatrixXf& N()                { return mN; }
    inline MatrixXf& LN()               { return mLN; }
    inline MatrixXf& V2D()              { return mV2D; }
    inline MatrixXf& rawPoints()        { return mRawPoints; }
    inline VectorXu& pathSegments()     { return mPathSegments; }
    inline VectorXu8& selectedPoints()  { return mSelectedPoints; }

    inline MatrixXu& F()                { return mF; }

    // Selection
    void centerAxisToSelection();
    Vector3f selectionCenter() const;
    void updatePointSelection();

private:
    // Raw sample data
    MatrixXu    mF;
    MatrixXf    mV2D;
    VectorXf    mH;
    VectorXf    mLH;
    MatrixXf    mN;
    MatrixXf    mLN;
    VectorXu    mPathSegments;
    // Untransformed data
    MatrixXf    mRawPoints;        // theta, phi, intensity
    PointsStats mPointsStats;

    // display Shaders
    nanogui::GLShader mMeshShader;
    nanogui::GLShader mShaders[VIEW_COUNT];
    std::function<void(
        const Vector3f&,   // view origin
        const Matrix4f&,   // model matrix
        const Matrix4f&,   // mvp matrix
        bool, std::shared_ptr<ColorMap>)> mDrawFunctions[VIEW_COUNT];
    nanogui::GLShader mPredictedOutgoingAngleShader;

    // display options
    bool mDisplayAsLog;
    bool mDisplayViews[VIEW_COUNT];

    // metadata
    Metadata mMetadata;
    Axis mAxis;

    // Selected point
    VectorXu8       mSelectedPoints;
    PointsStats     mSelectionStats;
};

TEKARI_NAMESPACE_END