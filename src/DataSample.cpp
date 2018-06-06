#include "tekari/DataSample.h"

using namespace std;
using namespace nanogui;

#define MAX_SELECTION_DISTANCE 30.0f

TEKARI_NAMESPACE_BEGIN

DataSample::DataSample()
:   mWaveLengthIndex(0)
,   mDisplayAsLog(false)
,   mDisplayViews{ false, false, true }
,   mPointsStats()
,   mSelectionStats()
,   mSelectionAxis{Vector3f{0.0f, 0.0f, 0.0f}}
,	mDirty(false)
{
    mDrawFunctions[PATH] = [this](const Matrix4f &mvp, std::shared_ptr<ColorMap>) {
        if (mDisplayViews[PATH])
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            mShaders[PATH].bind();
            mShaders[PATH].setUniform("modelViewProj", mvp);
            for (Eigen::Index i = 0; i < mPathSegments.size() - 1; ++i)
            {
                int offset = mPathSegments[i];
                int count = mPathSegments[i + 1] - mPathSegments[i] - 1;
                mShaders[PATH].drawArray(GL_LINE_STRIP, offset, count);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    };
    mDrawFunctions[POINTS] = [this](const Matrix4f &mvp, std::shared_ptr<ColorMap> colorMap) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        mShaders[POINTS].bind();
        colorMap->bind();
        mShaders[POINTS].setUniform("modelViewProj", mvp);
        mShaders[POINTS].setUniform("showAllPoints", mDisplayViews[POINTS]);
        mShaders[POINTS].drawArray(GL_POINTS, 0, mV2D.cols());
        glDisable(GL_BLEND);
    };
    mDrawFunctions[INCIDENT_ANGLE] = [this](const Matrix4f &mvp, std::shared_ptr<ColorMap>) {
        if (mDisplayViews[INCIDENT_ANGLE])
        {
            mShaders[INCIDENT_ANGLE].bind();
            mShaders[INCIDENT_ANGLE].setUniform("modelViewProj", mvp);
            mShaders[INCIDENT_ANGLE].drawArray(GL_POINTS, 0, 1);
        }
    };
}

DataSample::~DataSample()
{
    mMeshShader.free();
    mPredictedOutgoingAngleShader.free();
    for (int i = 0; i != VIEW_COUNT; ++i)
    {
        mShaders[i].free();
    }
}

void DataSample::drawGL(
    const Vector3f& viewOrigin,
    const Matrix4f& model,
    const Matrix4f& view,
    const Matrix4f& proj,
    int flags,
    shared_ptr<ColorMap> colorMap)
{
    // Every draw call requires depth testing
	glEnable(GL_DEPTH_TEST);

	// Precompute mvp
    Matrix4f mvp = proj * view * model;

	// Draw the mesh
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0, 2.0);
    mMeshShader.bind();
    colorMap->bind();
    mMeshShader.setUniform("modelViewProj", mvp);
    mMeshShader.setUniform("model", model);
    mMeshShader.setUniform("view", viewOrigin);
    mMeshShader.setUniform("useShadows", flags & USES_SHADOWS);
    mMeshShader.drawIndexed(GL_TRIANGLES, 0, mF.cols());
    glDisable(GL_POLYGON_OFFSET_FILL);

	// draw the predicted outgoing angle
    if (flags & DISPLAY_PREDICTED_OUTGOING_ANGLE)
    {
        mPredictedOutgoingAngleShader.bind();
        mPredictedOutgoingAngleShader.setUniform("modelViewProj", mvp);
        mPredictedOutgoingAngleShader.drawArray(GL_POINTS, 0, 1);
    }
	// Draw the axis if points are selected
    if (flags & DISPLAY_AXIS && hasSelection())
        mSelectionAxis.drawGL(mvp);

    for (const auto& drawFunc: mDrawFunctions)
        drawFunc(mvp, colorMap);

	// Don't forget to disable depth testing for later opengl draw calls
	glDisable(GL_DEPTH_TEST);
}

void DataSample::initShaders()
{
    const string shader_path = "../resources/shaders/";
    mMeshShader.initFromFiles("height_map", shader_path + "height_map.vert", shader_path + "height_map.frag");
    mShaders[PATH].initFromFiles("path", shader_path + "path.vert", shader_path + "path.frag");
    mShaders[POINTS].initFromFiles("points", shader_path + "points.vert", shader_path + "points.frag");
    mShaders[INCIDENT_ANGLE].initFromFiles("incident_angle", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");

    mPredictedOutgoingAngleShader.initFromFiles("predicted_outgoing_angle", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");
}

void DataSample::linkDataToShaders()
{
    if (mF.size() == 0)
    {
        throw runtime_error("ERROR: cannot link data to shader before loading data.");
    }

    mMeshShader.bind();
    mMeshShader.setUniform("color_map", 0);
	mMeshShader.uploadAttrib("in_pos2d", mV2D);
    mMeshShader.uploadAttrib("in_normal", currN());
    mMeshShader.uploadAttrib("in_height", currH());
    mMeshShader.uploadIndices(mF);

    mShaders[PATH].bind();
    mShaders[PATH].shareAttrib(mMeshShader, "in_pos2d");
    mShaders[PATH].shareAttrib(mMeshShader, "in_height");

    mShaders[POINTS].bind();
    mShaders[POINTS].setUniform("color_map", 0);
    mShaders[POINTS].shareAttrib(mMeshShader, "in_pos2d");
    mShaders[POINTS].shareAttrib(mMeshShader, "in_height");
    mShaders[POINTS].uploadAttrib("in_selected", mSelectedPoints);

    Vector2f origin2D = transformRawPoint({ mMetadata.incidentTheta(), mMetadata.incidentPhi() });
    Vector3f origin3D = Vector3f{ origin2D(0), 0.0f, origin2D(1) };
    mShaders[INCIDENT_ANGLE].bind();
    mShaders[INCIDENT_ANGLE].uploadAttrib("pos", Vector3f{ 0, 0, 0 });
    mShaders[INCIDENT_ANGLE].setUniform("origin", origin3D);
    mShaders[INCIDENT_ANGLE].setUniform("direction", Vector3f{ 0, 1, 0 });
    mShaders[INCIDENT_ANGLE].setUniform("color", Vector3f{ 1, 0, 1 });
    mShaders[INCIDENT_ANGLE].setUniform("length", 1.0f);

    origin3D = -origin3D;
    mPredictedOutgoingAngleShader.bind();
    mPredictedOutgoingAngleShader.uploadAttrib("pos", Vector3f{ 0, 0, 0 });
    mPredictedOutgoingAngleShader.setUniform("origin", origin3D);
    mPredictedOutgoingAngleShader.setUniform("direction", Vector3f{ 0, 1, 0 });
    mPredictedOutgoingAngleShader.setUniform("color", Vector3f{ 0, 0.1f, 0.8f });
    mPredictedOutgoingAngleShader.setUniform("length", 1.0f);

    mSelectionAxis.loadShader();
}

void DataSample::toggleLogView()
{
    mDisplayAsLog = !mDisplayAsLog;

    mMeshShader.bind();
    mMeshShader.uploadAttrib("in_normal", currN());
    mMeshShader.uploadAttrib("in_height", currH());

    mShaders[PATH].bind();
    mShaders[PATH].shareAttrib(mMeshShader, "in_height");
    mShaders[POINTS].bind();
    mShaders[POINTS].shareAttrib(mMeshShader, "in_height");

	if (hasSelection()) {
        update_selection_stats(mSelectionStats, mSelectedPoints, mRawPoints, mV2D, mDisplayAsLog ? mLH : mH);
		mSelectionAxis.setOrigin(selectionCenter());
	}
}

void DataSample::updatePointSelection()
{
    mShaders[POINTS].bind();
    mShaders[POINTS].uploadAttrib("in_selected", mSelectedPoints);

	update_selection_stats(mSelectionStats, mSelectedPoints, mRawPoints, mV2D, mDisplayAsLog ? mLH : mH);
	mSelectionAxis.setOrigin(selectionCenter());
}

void DataSample::setWaveLengthIndex(size_t displayedWaveLength)
{
    displayedWaveLength = std::min(displayedWaveLength, mH.size());
    if (mWaveLengthIndex == displayedWaveLength)
        return;

    mWaveLengthIndex = displayedWaveLength;

    mMeshShader.bind();
    mMeshShader.uploadAttrib("in_normal", currN());
    mMeshShader.uploadAttrib("in_height", currH());

    mShaders[PATH].bind();
    mShaders[PATH].shareAttrib(mMeshShader, "in_height");
    mShaders[POINTS].bind();
    mShaders[POINTS].shareAttrib(mMeshShader, "in_height");

	if (hasSelection()) {
		update_selection_stats(mSelectionStats, mSelectedPoints, mRawPoints, mV2D, mDisplayAsLog ? mLH : mH);
		mSelectionAxis.setOrigin(selectionCenter());
	}
}

TEKARI_NAMESPACE_END