#include "tekari/DataSample.h"

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <iostream>
#include <istream>

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
,	tri_delaunay2d(nullptr)
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
        m_Shaders[POINTS].drawArray(GL_POINTS, 0, tri_delaunay2d->num_points);
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
    for (int i = 0; i != VIEW_COUNT; ++i)
    {
        m_Shaders[i].free();
    }

    if (tri_delaunay2d)
    {
        tri_delaunay2d_release(tri_delaunay2d);
    }
}

void DataSample::drawGL(
    const Vector3f& viewOrigin,
    const Matrix4f& model,
    const Matrix4f& view,
    const Matrix4f& proj,
    bool useShadows, bool displayAxis,
    shared_ptr<ColorMap> colorMap)
{
    using namespace nanogui;
    if (tri_delaunay2d)
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
        m_MeshShader.setUniform("useShadows", useShadows);
        m_MeshShader.drawIndexed(GL_TRIANGLES, 0, tri_delaunay2d->num_triangles);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_POLYGON_OFFSET_FILL);

        for (int i = 0; i != VIEW_COUNT; ++i)
        {
            m_DrawFunctions[i](viewOrigin, model, mvp, useShadows, colorMap);
        }
        if (displayAxis)
        {
            m_Axis.drawGL(mvp);
        }
    }

}


void DataSample::triangulateData()
{
    if (tri_delaunay2d)
    {
        throw runtime_error("ERROR: cannot triangulate data twice!");
    }

    // triangulate vertx data
    delaunay2d_t *delaunay;
    delaunay = delaunay2d_from(m_2DPoints.data(), m_2DPoints.size());
    tri_delaunay2d = tri_delaunay2d_from(delaunay);
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
    stringstream rawMetaData;
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
        else if (*head == '#')
        {
            rawMetaData << head;
            m_Metadata.parse(head);
            //if (m_Metadata.datapointsInFile >= 0)
            //{
            //    // as soon as we know the total size of the dataset, reserve enough space for it
            //    m_2DPoints.reserve(m_Metadata.datapointsInFile);
            //    m_SelectedPoints.resize(m_Metadata.datapointsInFile, false);
            //    m_Heights.reserve(m_Metadata.datapointsInFile);
            //    m_LogHeights.reserve(m_Metadata.datapointsInFile);

            //    m_RawPoints.reserve(m_Metadata.datapointsInFile);
            //}
        }
        else
        {
            float phi, theta, intensity;
            if (sscanf(head, "%f %f %f", &theta, &phi, &intensity) != 3)
            {
                fclose(datasetFile);
                ostringstream errorMsg;
                errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                throw runtime_error(errorMsg.str());
            }
            m_RawPoints.push_back(Vector3f{ theta, phi, intensity });
            del_point2d_t transformedPoint = transformRawPoint(m_RawPoints.back());

            m_2DPoints.push_back(transformedPoint);
            m_Heights.push_back(intensity);
            m_LogHeights.push_back(intensity);
            
            m_PointsInfo.addPoint(m_2DPoints.size() - 1, m_RawPoints.back(), Vector3f{ transformedPoint.x, intensity, transformedPoint.y });

            m_SelectedPoints.push_back(false);
        }
    }
    m_PointsInfo.normalize();
    m_PointsInfo.normalizeAverage();
    fclose(datasetFile);

    // store raw metadata
    m_RawMetaData = rawMetaData.str();

    // normalize intensities
    float correction_factor = 0.0f;
    float min_intensity = m_PointsInfo.minIntensity();
    float max_intensity = m_PointsInfo.maxIntensity();
    if (min_intensity <= 0.0f)
        correction_factor = -min_intensity + 1e-10f;
    float min_log_intensity = log(min_intensity + correction_factor);
    float max_log_intensity = log(max_intensity);
    for (size_t i = 0; i < m_Heights.size(); ++i)
    {
        m_Heights[i] = (m_Heights[i] - min_intensity) / (max_intensity - min_intensity);
        m_LogHeights[i] = (log(m_LogHeights[i] + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
    }

    m_Axis.setOrigin(selectionCenter());
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
            m_2DPoints[lastValid] = m_2DPoints[i];
            m_Heights[lastValid] = m_Heights[i];
            m_LogHeights[lastValid] = m_LogHeights[i];
            m_RawPoints[lastValid] = m_RawPoints[i];
            m_SelectedPoints[lastValid] = 0;
            ++lastValid;

            // update point info
            m_PointsInfo.addPoint(i, m_RawPoints[i], Vector3f{ m_2DPoints[i].x, m_Heights[i], m_2DPoints[i].y });
        }
    }

    // resize all my vectors
    m_2DPoints.resize(m_PointsInfo.pointsCount());
    m_Heights.resize(m_PointsInfo.pointsCount());
    m_LogHeights.resize(m_PointsInfo.pointsCount());
    m_RawPoints.resize(m_PointsInfo.pointsCount());
    m_SelectedPoints.resize(m_PointsInfo.pointsCount());

    m_PointsInfo.normalize();
    m_Axis.setOrigin(m_PointsInfo.averagePoint());

    // recompute mesh
    tri_delaunay2d_release(tri_delaunay2d);
    tri_delaunay2d = nullptr;
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
    for (unsigned int i = 1; i < m_2DPoints.size(); ++i)
    {
        // if two last points are too far appart, a new path segments begins
        const del_point2d_t& current = m_2DPoints[i];
        const del_point2d_t& prev = m_2DPoints[i-1];
        float dx = prev.x - current.x;
        float dz = prev.y - current.y;
        if (dx * dx + dz * dz > MAX_SAMPLING_DISTANCE)
        {
            m_PathSegments.push_back(i);
        }
    }
    // path segments must always contain the last point
    m_PathSegments.push_back(m_2DPoints.size());
}

void DataSample::initShaders()
{
    const string shader_path = "../resources/shaders/";
    m_MeshShader.initFromFiles("height_map", shader_path + "height_map.vert", shader_path + "height_map.frag");
    m_Shaders[PATH].initFromFiles("path", shader_path + "path.vert", shader_path + "path.frag");
    m_Shaders[POINTS].initFromFiles("points", shader_path + "points.vert", shader_path + "points.frag");
    m_Shaders[INCIDENT_ANGLE].initFromFiles("incident_angle", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");
}

void DataSample::linkDataToShaders()
{
    if (!tri_delaunay2d)
    {
        throw runtime_error("ERROR: cannot link data to shader before loading data.");
    }
    if (m_ShaderLinked)
    {
        throw runtime_error("ERROR: cannot link data to shader twice.");
    }

    m_MeshShader.setUniform("color_map", 0);

    m_MeshShader.bind();
    m_MeshShader.uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_Normals.data());
    m_MeshShader.uploadAttrib("in_pos2d", tri_delaunay2d->num_points, 2, sizeof(del_point2d_t), GL_FLOAT, GL_FALSE, (const void*)tri_delaunay2d->points);
    m_MeshShader.uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_Heights.data());
    m_MeshShader.uploadAttrib("indices", tri_delaunay2d->num_triangles, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, tri_delaunay2d->tris);

    m_Shaders[PATH].bind();
    m_Shaders[PATH].shareAttrib(m_MeshShader, "in_pos2d");
    m_Shaders[PATH].shareAttrib(m_MeshShader, "in_height");

    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].shareAttrib(m_MeshShader, "in_pos2d");
    m_Shaders[POINTS].shareAttrib(m_MeshShader, "in_height");
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(unsigned char), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());

    Vector3f incidentCoords{ 0.0f, 0.0f, 0.0f };
    //(float)(m_Metadata.incidentTheta * cos(m_Metadata.incidentPhi * M_PI / 180.0f) / 90.0f),
                             //(float)(m_Metadata.incidentTheta * sin(m_Metadata.incidentPhi * M_PI / 180.0f) / 90.0f) };
    m_Shaders[INCIDENT_ANGLE].bind();
    m_Shaders[INCIDENT_ANGLE].uploadAttrib("pos", 1, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)&incidentCoords);
    m_Shaders[INCIDENT_ANGLE].setUniform("origin", incidentCoords);
    m_Shaders[INCIDENT_ANGLE].setUniform("direction", Vector3f{ 0, 1, 0 });
    m_Shaders[INCIDENT_ANGLE].setUniform("color", Vector3f{ 1, 0, 1 });
    m_Shaders[INCIDENT_ANGLE].setUniform("length", 1.0f);

    m_Axis.loadShader();

    m_ShaderLinked = true;
}

void DataSample::setDisplayAsLog(bool displayAsLog)
{
    if (m_DisplayAsLog == displayAsLog)
        return;

    m_DisplayAsLog = displayAsLog;
    vector<Vector3f> *normals = &m_Normals;
    vector<float>    *heights = &m_Heights;
    if (m_DisplayAsLog)
    {
        normals = &m_LogNormals;
        heights = &m_LogHeights;
    }

    m_MeshShader.bind();
    m_MeshShader.uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)normals->data());
    m_MeshShader.uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float),    GL_FLOAT, GL_FALSE, (const void*)heights->data());
    
    m_Shaders[PATH].bind();
    m_Shaders[PATH].shareAttrib(m_MeshShader, "in_height");
    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].shareAttrib(m_MeshShader, "in_height");
}

void DataSample::selectPoints(const Matrix4f & mvp, const SelectionBox& selectionBox,
    const Vector2i & canvasSize, SelectionMode mode)
{
    m_SelectedPointsInfo = PointSampleInfo();

    for (unsigned int i = 0; i < tri_delaunay2d->num_points; ++i)
    {
        Vector3f point = getVertex(i, m_DisplayAsLog);
        Vector4f projPoint = projectOnScreen(point, canvasSize, mvp);

        bool inSelection = selectionBox.contains(Vector2i{ projPoint[0], projPoint[1] });
        
        switch (mode)
        {
        case STANDARD:
            m_SelectedPoints[i] =  inSelection;
            break;
        case ADD:
            m_SelectedPoints[i] = inSelection || m_SelectedPoints[i];
            break;
        case SUBTRACT:
            m_SelectedPoints[i] = !inSelection && m_SelectedPoints[i];
            break;
        }
        // if we selected the point in the end
        if (m_SelectedPoints[i])
        {
            m_SelectedPointsInfo.addPoint(i, m_RawPoints[i], point);
        }
    }
    m_SelectedPointsInfo.normalize();
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
    for (unsigned int i = 0; i < tri_delaunay2d->num_points; ++i)
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
    }

    m_SelectedPointsInfo.normalize();
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
    memset(m_SelectedPoints.data(), 0, sizeof(unsigned char) * m_SelectedPoints.size());
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
    return {	tri_delaunay2d->points[i].x,
                logged ? m_LogHeights[i] : m_Heights[i],
                tri_delaunay2d->points[i].y };
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

    vector<Vector3f> &normals = logged ? m_LogNormals : m_Normals;
    normals[i0] += w0 * faceNormal;
    normals[i1] += w1 * faceNormal;
    normals[i2] += w2 * faceNormal;
}

void DataSample::computeNormals()
{
    m_Normals.resize(tri_delaunay2d->num_points);
    m_LogNormals.resize(tri_delaunay2d->num_points);
    memset(m_Normals.data(),     0, sizeof(Vector3f) * m_Normals.size());
    memset(m_LogNormals.data(),  0, sizeof(Vector3f) * m_LogNormals.size());

    for (unsigned int i = 0; i < tri_delaunay2d->num_triangles; ++i)
    {
        const unsigned int &i0 = tri_delaunay2d->tris[3 * i];
        const unsigned int &i1 = tri_delaunay2d->tris[3 * i + 1];
        const unsigned int &i2 = tri_delaunay2d->tris[3 * i + 2];

        computeTriangleNormal(i0, i1, i2, false);
        computeTriangleNormal(i0, i1, i2, true);
    }

    for (size_t i = 0; i < m_Normals.size(); ++i)
    {
        m_Normals[i].normalize();
        m_LogNormals[i].normalize();
    }
}

void DataSample::save(const std::string& path) const
{
    // try open file
    FILE* datasetFile = fopen(path.c_str(), "w");
    if (!datasetFile)
        throw runtime_error("Unable to open file " + path);

    // save metadata
    fprintf(datasetFile, "%s", m_RawMetaData.c_str());

    //!feof(datasetFile) && !ferror(datasetFile))
    for (unsigned int i = 0; i < m_RawPoints.size(); ++i)
    {
        fprintf(datasetFile, "%lf %lf %lf\n", m_RawPoints[i][0], m_RawPoints[i][1], m_RawPoints[i][2]);
    }
    fclose(datasetFile);
}

TEKARI_NAMESPACE_END