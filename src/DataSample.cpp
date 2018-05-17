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
:	m_DisplayAsLog(false)
,   m_DisplayViews{ false, false, true }
,	m_DelaunayTriangulation(nullptr)
,   m_PointsStats()
,   m_SelectionStats()
,   m_Axis{Vector3f{0.0f, 0.0f, 0.0f}}
{    
    m_DrawFunctions[PATH] = [this](const Vector3f&, const Matrix4f&,
        const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        if (m_DisplayViews[PATH])
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glEnable(GL_DEPTH_TEST);
            m_Shaders[PATH].bind();
            m_Shaders[PATH].setUniform("modelViewProj", mvp);
            for (unsigned int i = 0; i < m_PathSegments.size() - 1; ++i)
            {
                int offset = m_PathSegments[i];
                int count = m_PathSegments[i + 1] - m_PathSegments[i] - 1;
                m_Shaders[PATH].drawArray(GL_LINE_STRIP, offset, count);
            }
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    };
    m_DrawFunctions[POINTS] = [this](const Vector3f&, const Matrix4f&,
        const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_Shaders[POINTS].bind();
        m_Shaders[POINTS].setUniform("modelViewProj", mvp);
        m_Shaders[POINTS].setUniform("showAllPoints", m_DisplayViews[POINTS]);
        m_Shaders[POINTS].drawArray(GL_POINTS, 0, m_DelaunayTriangulation->num_points);
        glDisable(GL_BLEND);
    };
    m_DrawFunctions[INCIDENT_ANGLE] = [this](const Vector3f&, const Matrix4f&,
        const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        if (m_DisplayViews[INCIDENT_ANGLE])
        {
            glEnable(GL_DEPTH_TEST);
            m_Shaders[INCIDENT_ANGLE].bind();
            m_Shaders[INCIDENT_ANGLE].setUniform("modelViewProj", mvp);
            m_Shaders[INCIDENT_ANGLE].drawArray(GL_POINTS, 0, 1);
            glDisable(GL_DEPTH_TEST);
        }
    };

    // load vertex data from file
    PROFILE(readDataset(sampleDataPath));
}

DataSample::~DataSample()
{
    m_MeshShader.free();
    m_PredictedOutgoingAngleShader.free();
    for (int i = 0; i != VIEW_COUNT; ++i)
    {
        m_Shaders[i].free();
    }

    if (m_DelaunayTriangulation)
    {
        tri_delaunay2d_release(m_DelaunayTriangulation);
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
    if (m_DelaunayTriangulation)
    {
        Matrix4f mvp = proj * view * model;

        glEnable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_DEPTH_TEST);
        glPolygonOffset(2.0, 2.0);
        m_MeshShader.bind();
        colorMap->bind();
        m_MeshShader.setUniform("modelViewProj", mvp);
        m_MeshShader.setUniform("model", model);
        m_MeshShader.setUniform("view", viewOrigin);
        m_MeshShader.setUniform("useShadows", flags & USES_SHADOWS);
        m_MeshShader.drawIndexed(GL_TRIANGLES, 0, m_DelaunayTriangulation->num_triangles);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_POLYGON_OFFSET_FILL);

        if (flags & DISPLAY_PREDICTED_OUTGOING_ANGLE)
        {
            glEnable(GL_DEPTH_TEST);
            m_PredictedOutgoingAngleShader.bind();
            m_PredictedOutgoingAngleShader.setUniform("modelViewProj", mvp);
            m_PredictedOutgoingAngleShader.drawArray(GL_POINTS, 0, 1);
            glDisable(GL_DEPTH_TEST);
        }

        for (int i = 0; i != VIEW_COUNT; ++i)
        {
            m_DrawFunctions[i](viewOrigin, model, mvp, flags & USES_SHADOWS, colorMap);
        }
        if (flags & DISPLAY_AXIS)
        {
            m_Axis.drawGL(mvp);
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
            m_Metadata.addLine(head);
        }
        else
        {
            if (readingMetadata)
            {
                readingMetadata = false;
                m_Metadata.initInfos();
                if (m_Metadata.pointsInFile() >= 0)
                {
                    // as soon as we know the total size of the dataset, reserve enough space for it
                    m_V2D.reserve(m_Metadata.pointsInFile());
                    m_SelectedPoints.reserve(m_Metadata.pointsInFile());
                    m_RawPoints.reserve(m_Metadata.pointsInFile());
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
            m_RawPoints.push_back(rawPoint);
            m_V2D.push_back(transformedPoint);
            m_SelectedPoints.push_back(false);
        }
    }
    fclose(datasetFile);
}

void DataSample::initShaders()
{
    const string shader_path = "../resources/shaders/";
    m_MeshShader.initFromFiles("height_map", shader_path + "height_map.vert", shader_path + "height_map.frag");
    m_Shaders[PATH].initFromFiles("path", shader_path + "path.vert", shader_path + "path.frag");
    m_Shaders[POINTS].initFromFiles("points", shader_path + "points.vert", shader_path + "points.frag");
    m_Shaders[INCIDENT_ANGLE].initFromFiles("incident_angle", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");

    m_PredictedOutgoingAngleShader.initFromFiles("predicted_outgoing_angle", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");
}

void DataSample::linkDataToShaders()
{
    if (!m_DelaunayTriangulation)
    {
        throw runtime_error("ERROR: cannot link data to shader before loading data.");
    }

    m_MeshShader.setUniform("color_map", 0);

    vector<float>& heights = m_DisplayAsLog ? m_LH : m_H;

    m_MeshShader.bind();
    m_MeshShader.uploadAttrib("in_normal", m_DelaunayTriangulation->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_N.data());
    m_MeshShader.uploadAttrib("in_pos2d", m_DelaunayTriangulation->num_points, 2, sizeof(del_point2d_t), GL_FLOAT, GL_FALSE, (const void*)m_DelaunayTriangulation->points);
    m_MeshShader.uploadAttrib("in_height", m_DelaunayTriangulation->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)heights.data());
    m_MeshShader.uploadAttrib("indices", m_DelaunayTriangulation->num_triangles, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, m_DelaunayTriangulation->tris);

    m_Shaders[PATH].bind();
    m_Shaders[PATH].shareAttrib(m_MeshShader, "in_pos2d");
    m_Shaders[PATH].shareAttrib(m_MeshShader, "in_height");

    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].shareAttrib(m_MeshShader, "in_pos2d");
    m_Shaders[POINTS].shareAttrib(m_MeshShader, "in_height");
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(uint8_t), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());

    Vector3f pos{ 0.0f, 0.0f, 0.0f };
    del_point2d_t origin = transformRawPoint({ m_Metadata.incidentTheta(), m_Metadata.incidentPhi(), 0.0f });
    m_Shaders[INCIDENT_ANGLE].bind();
    m_Shaders[INCIDENT_ANGLE].uploadAttrib("pos", 1, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)&pos);
    m_Shaders[INCIDENT_ANGLE].setUniform("origin", Vector3f{ origin.x, 0.0f, origin.y });
    m_Shaders[INCIDENT_ANGLE].setUniform("direction", Vector3f{ 0, 1, 0 });
    m_Shaders[INCIDENT_ANGLE].setUniform("color", Vector3f{ 1, 0, 1 });
    m_Shaders[INCIDENT_ANGLE].setUniform("length", 1.0f);

    origin = { -origin.x, -origin.y };
    m_PredictedOutgoingAngleShader.bind();
    m_PredictedOutgoingAngleShader.uploadAttrib("pos", 1, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)&pos);
    m_PredictedOutgoingAngleShader.setUniform("origin", Vector3f{ origin.x, 0.0f, origin.y });
    m_PredictedOutgoingAngleShader.setUniform("direction", Vector3f{ 0, 1, 0 });
    m_PredictedOutgoingAngleShader.setUniform("color", Vector3f{ 0, 0.1f, 0.8f });
    m_PredictedOutgoingAngleShader.setUniform("length", 1.0f);

    m_Axis.loadShader();
}

void DataSample::setDisplayAsLog(bool displayAsLog)
{
    if (m_DisplayAsLog == displayAsLog)
        return;

    m_DisplayAsLog = displayAsLog;
    vector<Vector3f> &normals = m_DisplayAsLog ? m_LN : m_N;
    vector<float>    &heights = m_DisplayAsLog ? m_LH : m_H;

    m_MeshShader.bind();
    m_MeshShader.uploadAttrib("in_normal", m_DelaunayTriangulation->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)normals.data());
    m_MeshShader.uploadAttrib("in_height", m_DelaunayTriangulation->num_points, 1, sizeof(float),    GL_FLOAT, GL_FALSE, (const void*)heights.data());
    
    m_Shaders[PATH].bind();
    m_Shaders[PATH].shareAttrib(m_MeshShader, "in_height");
    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].shareAttrib(m_MeshShader, "in_height");

    update_selection_stats(m_SelectionStats, m_SelectedPoints, m_RawPoints, m_V2D, m_DisplayAsLog ? m_LH : m_H);
    centerAxisToSelection();
}

void DataSample::updatePointSelection()
{
    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(uint8_t), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());

    update_selection_stats(m_SelectionStats, m_SelectedPoints, m_RawPoints, m_V2D, m_DisplayAsLog ? m_LH : m_H);
    centerAxisToSelection();
}

Vector3f DataSample::selectionCenter() const
{
    return m_SelectionStats.pointsCount() == 0 ? m_PointsStats.averagePoint() : m_SelectionStats.averagePoint();
}

void DataSample::centerAxisToSelection()
{
    m_Axis.setOrigin(selectionCenter());
}

void DataSample::save(const std::string& path) const
{
    // try open file
    FILE* datasetFile = fopen(path.c_str(), "w");
    if (!datasetFile)
        throw runtime_error("Unable to open file " + path);

    // save metadata
    fprintf(datasetFile, "%s", m_Metadata.toString().c_str());

    //!feof(datasetFile) && !ferror(datasetFile))
    for (unsigned int i = 0; i < m_RawPoints.size(); ++i)
    {
        fprintf(datasetFile, "%lf %lf %lf\n", m_RawPoints[i][0], m_RawPoints[i][1], m_RawPoints[i][2]);
    }
    fclose(datasetFile);
}

TEKARI_NAMESPACE_END