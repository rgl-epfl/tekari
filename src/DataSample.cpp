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

#define MAX_SAMPLING_DISTANCE 0.05f
#define MAX_SELECTION_DISTANCE 30.0f

TEKARI_NAMESPACE_BEGIN

DataSample::DataSample(const string& sampleDataPath)
:	m_DisplayAsLog(false)
,   m_DisplayViews{ false, false, true }
,   m_ShaderLinked(false)
,	m_DelaunayTriangulation(nullptr)
,   m_PointsInfo()
,   m_SelectedPointsInfo()
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

    PROFILE(triangulateData());
    PROFILE(computePathSegments());
    PROFILE(computeNormals());
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

void DataSample::triangulateData()
{
    if (m_DelaunayTriangulation)
    {
        throw runtime_error("ERROR: cannot triangulate data twice!");
    }

    // triangulate vertx data
    delaunay2d_t *delaunay;
    delaunay = delaunay2d_from(m_2DV.data(), m_2DV.size());
    m_DelaunayTriangulation = tri_delaunay2d_from(delaunay);
    delaunay2d_release(delaunay);
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
                    m_2DV.reserve(m_Metadata.pointsInFile());
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
            m_2DV.push_back(transformedPoint);
            m_SelectedPoints.push_back(false);
            m_PointsInfo.addPoint(m_2DV.size() - 1, rawPoint, Vector3f{ transformedPoint.x, intensity, transformedPoint.y });
        }
    }
    fclose(datasetFile);

    m_PointsInfo.normalize();
    m_PointsInfo.normalizeAverage();

    computeNormalizedHeights();
    m_Axis.setOrigin(selectionCenter());
}

void DataSample::computeNormalizedHeights()
{
    m_H.resize(m_RawPoints.size());
    m_LH.resize(m_RawPoints.size());

    // normalize intensities
    float min_intensity = m_PointsInfo.minIntensity();
    float max_intensity = m_PointsInfo.maxIntensity();
    float correction_factor = 0.0f;
    if (min_intensity <= 0.0f)
        correction_factor = -min_intensity + 1e-10f;
    float min_log_intensity = log(min_intensity + correction_factor);
    float max_log_intensity = log(max_intensity);
    
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)m_RawPoints.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
            for (uint32_t i = range.begin(); i < range.end(); ++i)
            {
                m_H[i] = (m_RawPoints[i][2] - min_intensity) / (max_intensity - min_intensity);
                m_LH[i] = (log(m_RawPoints[i][2] + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
            }
        }
    );
}

bool DataSample::deleteSelectedPoints()
{
    // if no points selected, there's nothing to do
    if (m_SelectedPointsInfo.pointsCount() == 0)
        return false;

    // delete per vertex data
    m_PointsInfo = PointSampleInfo();
    m_SelectedPointsInfo = PointSampleInfo();
    
    unsigned int lastValid = 0;
    for (unsigned int i = 0; i < m_SelectedPoints.size(); ++i)
    {
        if (!m_SelectedPoints[i])
        {
            // move undeleted point to last valid position
            m_2DV[lastValid] = m_2DV[i];
            m_RawPoints[lastValid] = m_RawPoints[i];
            m_SelectedPoints[lastValid] = 0;
            ++lastValid;

            // update point info
            m_PointsInfo.addPoint(i, m_RawPoints[i], Vector3f{ m_2DV[i].x, m_H[i], m_2DV[i].y });
        }
    }

    // resize vectors
    m_2DV.resize(m_PointsInfo.pointsCount());
    m_RawPoints.resize(m_PointsInfo.pointsCount());
    m_SelectedPoints.resize(m_PointsInfo.pointsCount());

    computeNormalizedHeights();

    m_PointsInfo.normalize();
    m_Axis.setOrigin(m_PointsInfo.averagePoint());

    // recompute mesh
    tri_delaunay2d_release(m_DelaunayTriangulation);
    m_DelaunayTriangulation = nullptr;
    PROFILE(triangulateData());
    PROFILE(computePathSegments());
    PROFILE(computeNormals());

    m_ShaderLinked = false;
    linkDataToShaders();
    return true;
}

void DataSample::computePathSegments()
{
    m_PathSegments.clear();
    // path segments must always contain the first point
    m_PathSegments.push_back(0);
    for (unsigned int i = 1; i < m_2DV.size(); ++i)
    {
        // if two last points are too far appart, a new path segments begins
        const del_point2d_t& current = m_2DV[i];
        const del_point2d_t& prev = m_2DV[i-1];
        float dx = prev.x - current.x;
        float dz = prev.y - current.y;
        if (dx * dx + dz * dz > MAX_SAMPLING_DISTANCE)
        {
            m_PathSegments.push_back(i);
        }
    }
    // path segments must always contain the last point
    m_PathSegments.push_back(m_2DV.size());
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
    if (m_ShaderLinked)
    {
        throw runtime_error("ERROR: cannot link data to shader twice.");
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
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(unsigned char), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());

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

    m_ShaderLinked = true;
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

    updateSelectionInfo();
    m_Axis.setOrigin(selectionCenter());
}

void DataSample::selectPoints(const Matrix4f & mvp, const SelectionBox& selectionBox,
    const Vector2i & canvasSize, SelectionMode mode)
{
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)m_DelaunayTriangulation->num_points, GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            Vector3f point = getVertex(i, m_DisplayAsLog);
            Vector4f projPoint = projectOnScreen(point, canvasSize, mvp);

            bool inSelection = selectionBox.contains(Vector2i{ projPoint[0], projPoint[1] });

            switch (mode)
            {
            case STANDARD:
                m_SelectedPoints[i] = inSelection;
                break;
            case ADD:
                m_SelectedPoints[i] = inSelection || m_SelectedPoints[i];
                break;
            case SUBTRACT:
                m_SelectedPoints[i] = !inSelection && m_SelectedPoints[i];
                break;
            }
        }
    });
    updateSelectionInfo();
    m_Axis.setOrigin(selectionCenter());

    updatePointSelection();
}


void DataSample::selectSinglePoint(const nanogui::Matrix4f& mvp,
    const nanogui::Vector2i & mousePos,
    const nanogui::Vector2i & canvasSize)
{
    m_SelectedPointsInfo = PointSampleInfo();

    float smallestDistance = MAX_SELECTION_DISTANCE * MAX_SELECTION_DISTANCE;
    int closestPointIndex = -1;
    for (unsigned int i = 0; i < m_DelaunayTriangulation->num_points; ++i)
    {
        Vector3f point = getVertex(i, m_DisplayAsLog);
        Vector4f projPoint = projectOnScreen(point, canvasSize, mvp);

        float distSqr = Vector2f{ projPoint[0] - mousePos[0], projPoint[1] - mousePos[1] }.squaredNorm();
        
        if (smallestDistance > distSqr)
        {
            closestPointIndex = i;
            smallestDistance = distSqr;
        }

        m_SelectedPoints[i] = false;
    }
    if (closestPointIndex != -1)
    {
        m_SelectedPoints[closestPointIndex] = true;
        m_SelectedPointsInfo.addPoint(closestPointIndex, m_RawPoints[closestPointIndex], getVertex(closestPointIndex, m_DisplayAsLog));
        m_SelectedPointsInfo.normalize();
    }

    m_Axis.setOrigin(selectionCenter());
    updatePointSelection();
}

void DataSample::updatePointSelection()
{
    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(unsigned char), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());
}

void DataSample::deselectAllPoints()
{
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)m_DelaunayTriangulation->num_points, GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
            m_SelectedPoints[i] = 0;
    });
    updatePointSelection();

    m_SelectedPointsInfo = PointSampleInfo();
    m_Axis.setOrigin(selectionCenter());
}

void DataSample::movePointsAlongPath(bool up)
{
    unsigned char extremity;
    if (up)
    {
        extremity = m_SelectedPoints.back();
        memmove(m_SelectedPoints.data()+1, m_SelectedPoints.data(), m_SelectedPoints.size() - 1);
        m_SelectedPoints.front() = extremity;
    }
    else
    {
        extremity = m_SelectedPoints.front();
        memmove(m_SelectedPoints.data(), m_SelectedPoints.data()+1, m_SelectedPoints.size() - 1);
        m_SelectedPoints.back() = extremity;
    }
    updatePointSelection();
    updateSelectionInfo();
    m_Axis.setOrigin(selectionCenter());
}

void DataSample::selectHighestPoint()
{
    int highestPointIndex = m_SelectedPointsInfo.pointsCount() == 0 ?
                            m_PointsInfo.highestPointIndex() :
                            m_SelectedPointsInfo.highestPointIndex();

    m_SelectedPointsInfo = PointSampleInfo();

    memset(m_SelectedPoints.data(), 0, sizeof(unsigned char) * m_SelectedPoints.size());
    m_SelectedPoints[highestPointIndex] = 1;
    updatePointSelection();

    m_SelectedPointsInfo.addPoint(  highestPointIndex,
                                    m_RawPoints[highestPointIndex],
                                    getVertex(highestPointIndex, false));

    m_SelectedPointsInfo.normalize();
    m_Axis.setOrigin(selectionCenter());
}

void DataSample::updateSelectionInfo()
{
    m_SelectedPointsInfo = PointSampleInfo();
    for (unsigned int i = 0; i < m_SelectedPoints.size(); ++i)
    {
        if (m_SelectedPoints[i])
        {
            m_SelectedPointsInfo.addPoint(i, m_RawPoints[i], getVertex(i, m_DisplayAsLog));
        }
    }
    m_SelectedPointsInfo.normalize();
}

nanogui::Vector3f DataSample::selectionCenter()
{
    return m_SelectedPointsInfo.pointsCount() == 0 ? m_PointsInfo.averagePoint() : m_SelectedPointsInfo.averagePoint();
}

Vector3f DataSample::getVertex(unsigned int i, bool logged) const
{
    return {	m_DelaunayTriangulation->points[i].x,
                logged ? m_LH[i] : m_H[i],
                m_DelaunayTriangulation->points[i].y };
}

void DataSample::computeTriangleNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged)
{
    const Vector3f e01 = (getVertex(i1, logged) - getVertex(i0, logged)).normalized();
    const Vector3f e12 = (getVertex(i2, logged) - getVertex(i1, logged)).normalized();
    const Vector3f e20 = (getVertex(i0, logged) - getVertex(i2, logged)).normalized();

    Vector3f faceNormal = e12.cross(-e01).normalized();

    float w0 = (float)acos(max(-1.0f, min(1.0f, e01.dot(-e20))));
    float w1 = (float)acos(max(-1.0f, min(1.0f, e12.dot(-e01))));
    float w2 = (float)acos(max(-1.0f, min(1.0f, e20.dot(-e12))));

    vector<Vector3f> &normals = logged ? m_LN : m_N;
    normals[i0] += w0 * faceNormal;
    normals[i1] += w1 * faceNormal;
    normals[i2] += w2 * faceNormal;
}

void DataSample::computeNormals()
{
    m_N.resize(m_DelaunayTriangulation->num_points);
    m_LN.resize(m_DelaunayTriangulation->num_points);
    memset(m_N.data(),     0, sizeof(Vector3f) * m_N.size());
    memset(m_LN.data(),  0, sizeof(Vector3f) * m_LN.size());

    for (unsigned int i = 0; i < m_DelaunayTriangulation->num_triangles; ++i)
    {
        const unsigned int &i0 = m_DelaunayTriangulation->tris[3 * i];
        const unsigned int &i1 = m_DelaunayTriangulation->tris[3 * i + 1];
        const unsigned int &i2 = m_DelaunayTriangulation->tris[3 * i + 2];

        computeTriangleNormal(i0, i1, i2, false);
        computeTriangleNormal(i0, i1, i2, true);
    }

    for (size_t i = 0; i < m_N.size(); ++i)
    {
        m_N[i].normalize();
        m_LN[i].normalize();
    }
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