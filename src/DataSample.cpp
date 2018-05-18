#include "tekari/DataSample.h"

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <iostream>
#include <istream>
#include <tbb/tbb.h>

#include "tekari/delaunay.h"
#include "tekari/stop_watch.h"

using namespace std;
using namespace nanogui;

#define MAX_SELECTION_DISTANCE 30.0f

TEKARI_NAMESPACE_BEGIN

DataSample::DataSample(const string& sampleDataPath)
:	mDisplayAsLog(false)
,   mDisplayViews{ false, false, true }
,	mDelaunayTriangulation(nullptr)
,   mPointsStats()
,   mSelectionStats()
,   mAxis{Vector3f{0.0f, 0.0f, 0.0f}}
{    
    mDrawFunctions[PATH] = [this](const Vector3f&, const Matrix4f&,
        const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        if (mDisplayViews[PATH])
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glEnable(GL_DEPTH_TEST);
            mShaders[PATH].bind();
            mShaders[PATH].setUniform("modelViewProj", mvp);
            for (unsigned int i = 0; i < mPathSegments.size() - 1; ++i)
            {
                int offset = mPathSegments[i];
                int count = mPathSegments[i + 1] - mPathSegments[i] - 1;
                mShaders[PATH].drawArray(GL_LINE_STRIP, offset, count);
            }
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    };
    mDrawFunctions[POINTS] = [this](const Vector3f&, const Matrix4f&,
        const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        mShaders[POINTS].bind();
        mShaders[POINTS].setUniform("modelViewProj", mvp);
        mShaders[POINTS].setUniform("showAllPoints", mDisplayViews[POINTS]);
        mShaders[POINTS].drawArray(GL_POINTS, 0, mDelaunayTriangulation->num_points);
        glDisable(GL_BLEND);
    };
    mDrawFunctions[INCIDENT_ANGLE] = [this](const Vector3f&, const Matrix4f&,
        const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        if (mDisplayViews[INCIDENT_ANGLE])
        {
            glEnable(GL_DEPTH_TEST);
            mShaders[INCIDENT_ANGLE].bind();
            mShaders[INCIDENT_ANGLE].setUniform("modelViewProj", mvp);
            mShaders[INCIDENT_ANGLE].drawArray(GL_POINTS, 0, 1);
            glDisable(GL_DEPTH_TEST);
        }
    };

    // load vertex data from file
    PROFILE(readDataset(sampleDataPath));
}

DataSample::~DataSample()
{
    mMeshShader.free();
    mPredictedOutgoingAngleShader.free();
    for (int i = 0; i != VIEW_COUNT; ++i)
    {
        mShaders[i].free();
    }

    if (mDelaunayTriangulation)
    {
        tri_delaunay2d_release(mDelaunayTriangulation);
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
    if (mDelaunayTriangulation)
    {
        Matrix4f mvp = proj * view * model;

        glEnable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_DEPTH_TEST);
        glPolygonOffset(2.0, 2.0);
        mMeshShader.bind();
        colorMap->bind();
        mMeshShader.setUniform("modelViewProj", mvp);
        mMeshShader.setUniform("model", model);
        mMeshShader.setUniform("view", viewOrigin);
        mMeshShader.setUniform("useShadows", flags & USES_SHADOWS);
        mMeshShader.drawIndexed(GL_TRIANGLES, 0, mDelaunayTriangulation->num_triangles);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_POLYGON_OFFSET_FILL);

        if (flags & DISPLAY_PREDICTED_OUTGOING_ANGLE)
        {
            glEnable(GL_DEPTH_TEST);
            mPredictedOutgoingAngleShader.bind();
            mPredictedOutgoingAngleShader.setUniform("modelViewProj", mvp);
            mPredictedOutgoingAngleShader.drawArray(GL_POINTS, 0, 1);
            glDisable(GL_DEPTH_TEST);
        }

        for (int i = 0; i != VIEW_COUNT; ++i)
        {
            mDrawFunctions[i](viewOrigin, model, mvp, flags & USES_SHADOWS, colorMap);
        }
        if (flags & DISPLAY_AXIS)
        {
            mAxis.drawGL(mvp);
        }
    }

}

void DataSample::readDataset(const string &filePath)
{
    // try open file
    FILE* datasetFile = fopen(filePath.c_str(), "r");
    if (!datasetFile)
        throw runtime_error("Unable to open file " + filePath);

    unsigned int lineNumber = 0;
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
    bool readingMetadata = true;
    while (!feof(datasetFile) && !ferror(datasetFile) && fgets(line, MAX_LENGTH, datasetFile))
    {
        ++lineNumber;
        
        // remove any trailing spaces (this will detect full of spaces lines)
        const char* head = line;
        while (isspace(*head)) ++head;

        if (*head == '\0')
        {
            // skip empty lines
        }
        else if (*head == '#' && readingMetadata)
        {
            mMetadata.addLine(head);
        }
        else
        {
            if (readingMetadata)
            {
                readingMetadata = false;
                mMetadata.initInfos();
                if (mMetadata.pointsInFile() >= 0)
                {
                    // as soon as we know the total size of the dataset, reserve enough space for it
                    mV2D.reserve(mMetadata.pointsInFile());
                    mSelectedPoints.reserve(mMetadata.pointsInFile());
                    mRawPoints.reserve(mMetadata.pointsInFile());
                }
            }
            float phi, theta, intensity;
            if (sscanf(head, "%f %f %f", &theta, &phi, &intensity) != 3)
            {
                fclose(datasetFile);
                ostringstream errorMsg;
                errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                throw runtime_error(errorMsg.str());
            }
            Vector3f rawPoint = Vector3f{ theta, phi, intensity };
            del_point2d_t transformedPoint = transformRawPoint(rawPoint);
            mRawPoints.push_back(rawPoint);
            mV2D.push_back(transformedPoint);
            mSelectedPoints.push_back(false);
        }
    }
    fclose(datasetFile);
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
    if (!mDelaunayTriangulation)
    {
        throw runtime_error("ERROR: cannot link data to shader before loading data.");
    }

    mMeshShader.setUniform("color_map", 0);

    VectorXf& heights = mDisplayAsLog ? mLH : mH;

    mMeshShader.bind();
    mMeshShader.uploadAttrib("in_normal", mDelaunayTriangulation->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)mN.data());
    mMeshShader.uploadAttrib("in_pos2d", mDelaunayTriangulation->num_points, 2, sizeof(del_point2d_t), GL_FLOAT, GL_FALSE, (const void*)mDelaunayTriangulation->points);
    mMeshShader.uploadAttrib("in_height", mDelaunayTriangulation->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)heights.data());
    mMeshShader.uploadAttrib("indices", mDelaunayTriangulation->num_triangles, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, mDelaunayTriangulation->tris);

    mShaders[PATH].bind();
    mShaders[PATH].shareAttrib(mMeshShader, "in_pos2d");
    mShaders[PATH].shareAttrib(mMeshShader, "in_height");

    mShaders[POINTS].bind();
    mShaders[POINTS].shareAttrib(mMeshShader, "in_pos2d");
    mShaders[POINTS].shareAttrib(mMeshShader, "in_height");
    mShaders[POINTS].uploadAttrib("in_selected", mSelectedPoints.size(), 1, sizeof(uint8_t), GL_BYTE, GL_FALSE, (const void*)mSelectedPoints.data());

    Vector3f pos{ 0.0f, 0.0f, 0.0f };
    del_point2d_t origin = transformRawPoint({ mMetadata.incidentTheta(), mMetadata.incidentPhi(), 0.0f });
    mShaders[INCIDENT_ANGLE].bind();
    mShaders[INCIDENT_ANGLE].uploadAttrib("pos", 1, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)&pos);
    mShaders[INCIDENT_ANGLE].setUniform("origin", Vector3f{ origin.x, 0.0f, origin.y });
    mShaders[INCIDENT_ANGLE].setUniform("direction", Vector3f{ 0, 1, 0 });
    mShaders[INCIDENT_ANGLE].setUniform("color", Vector3f{ 1, 0, 1 });
    mShaders[INCIDENT_ANGLE].setUniform("length", 1.0f);

    origin = { -origin.x, -origin.y };
    mPredictedOutgoingAngleShader.bind();
    mPredictedOutgoingAngleShader.uploadAttrib("pos", 1, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)&pos);
    mPredictedOutgoingAngleShader.setUniform("origin", Vector3f{ origin.x, 0.0f, origin.y });
    mPredictedOutgoingAngleShader.setUniform("direction", Vector3f{ 0, 1, 0 });
    mPredictedOutgoingAngleShader.setUniform("color", Vector3f{ 0, 0.1f, 0.8f });
    mPredictedOutgoingAngleShader.setUniform("length", 1.0f);

    mAxis.loadShader();
}

void DataSample::setDisplayAsLog(bool displayAsLog)
{
    if (mDisplayAsLog == displayAsLog)
        return;

    mDisplayAsLog = displayAsLog;

    mMeshShader.bind();
    mMeshShader.uploadAttrib("in_normal", mDelaunayTriangulation->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)currN().data());
    mMeshShader.uploadAttrib("in_height", mDelaunayTriangulation->num_points, 1, sizeof(float),    GL_FLOAT, GL_FALSE, (const void*)currH().data());
    
    mShaders[PATH].bind();
    mShaders[PATH].shareAttrib(mMeshShader, "in_height");
    mShaders[POINTS].bind();
    mShaders[POINTS].shareAttrib(mMeshShader, "in_height");

    update_selection_stats(mSelectionStats, mSelectedPoints, mRawPoints, mV2D, currH());
    centerAxisToSelection();
}

void DataSample::updatePointSelection()
{
    mShaders[POINTS].bind();
    mShaders[POINTS].uploadAttrib("in_selected", mSelectedPoints.size(), 1, sizeof(uint8_t), GL_BYTE, GL_FALSE, (const void*)mSelectedPoints.data());

    update_selection_stats(mSelectionStats, mSelectedPoints, mRawPoints, mV2D, mDisplayAsLog ? mLH : mH);
    centerAxisToSelection();
}

Vector3f DataSample::selectionCenter() const
{
    return mSelectionStats.pointsCount() == 0 ? mPointsStats.averagePoint() : mSelectionStats.averagePoint();
}

void DataSample::centerAxisToSelection()
{
    mAxis.setOrigin(selectionCenter());
}

void DataSample::save(const std::string& path) const
{
    // try open file
    FILE* datasetFile = fopen(path.c_str(), "w");
    if (!datasetFile)
        throw runtime_error("Unable to open file " + path);

    // save metadata
    fprintf(datasetFile, "%s", mMetadata.toString().c_str());

    //!feof(datasetFile) && !ferror(datasetFile))
    for (unsigned int i = 0; i < mRawPoints.size(); ++i)
    {
        fprintf(datasetFile, "%lf %lf %lf\n", mRawPoints[i][0], mRawPoints[i][1], mRawPoints[i][2]);
    }
    fclose(datasetFile);
}

TEKARI_NAMESPACE_END