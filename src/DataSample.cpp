#include "DataSample.h"

#include <cstdint>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <iostream>
#include <istream>
#include <delaunay.h>

#include "common.h"
#include "stop_watch.h"

using namespace std;
using namespace nanogui;

#define MAX_SAMPLING_DISTANCE 0.05f

DataSample::DataSample()
:	m_DisplayViews{ true, false, false }
,   m_ShaderLinked(false)
,	tri_delaunay2d(nullptr)
,   m_PointsInfo()
,   m_SelectedPointsInfo()
,   m_Axis{Vector3f{0.0f, 0.0f, 0.0f}}
{
    m_DrawFunctions[NORMAL] = m_DrawFunctions[LOG] = [this](Views view, const Vector3f& viewOrigin, const Matrix4f& model,
        const Matrix4f&, const Matrix4f&, const Matrix4f &mvp, bool useShadows, shared_ptr<ColorMap> colorMap) {
        if (m_DisplayViews[view])
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glEnable(GL_DEPTH_TEST);
            glPolygonOffset(2.0, 2.0);
            m_Shaders[view].bind();
            colorMap->bind();
            m_Shaders[view].setUniform("modelViewProj", mvp);
            m_Shaders[view].setUniform("model", model);
            m_Shaders[view].setUniform("view", viewOrigin);
            m_Shaders[view].setUniform("useShadows", useShadows);
            m_Shaders[view].drawIndexed(GL_TRIANGLES, 0, tri_delaunay2d->num_triangles);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    };
    m_DrawFunctions[PATH] = [this](Views view, const Vector3f&, const Matrix4f&,
        const Matrix4f&, const Matrix4f&, const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        if (m_DisplayViews[view])
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glEnable(GL_DEPTH_TEST);
            m_Shaders[view].bind();
            m_Shaders[view].setUniform("modelViewProj", mvp);
            for (unsigned int i = 0; i < m_PathSegments.size() - 1; ++i)
            {
                int offset = m_PathSegments[i];
                int count = m_PathSegments[i + 1] - m_PathSegments[i] - 1;
                m_Shaders[view].drawArray(GL_LINE_STRIP, offset, count);
            }
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    };

    m_DrawFunctions[POINTS] = [this](Views view, const Vector3f&, const Matrix4f&,
        const Matrix4f&, const Matrix4f&, const Matrix4f &mvp, bool, shared_ptr<ColorMap>) {
        glPointSize(4);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_Shaders[view].bind();
        m_Shaders[view].setUniform("modelViewProj", mvp);
        m_Shaders[view].setUniform("showAllPoints", m_DisplayViews[view]);
        m_Shaders[view].drawArray(GL_POINTS, 0, tri_delaunay2d->num_points);
        glDisable(GL_BLEND);
    };
}

DataSample::DataSample(const string& sampleDataPath)
:	DataSample()
{
    loadFromFile(sampleDataPath);
}

DataSample::~DataSample()
{
    for (int i = NORMAL; i != VIEW_COUNT; ++i)
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
    bool useShadows,
    shared_ptr<ColorMap> colorMap)
{
    using namespace nanogui;
    if (tri_delaunay2d)
    {
        Matrix4f mvp = proj * view * model;

        for (int i = NORMAL; i != VIEW_COUNT; ++i)
        {
            m_DrawFunctions[i](static_cast<Views>(i), viewOrigin, model, view, proj, mvp, useShadows, colorMap);
        }

        m_Axis.drawGL(mvp);
    }

}

void DataSample::loadFromFile(const string& sampleDataPath)
{
    if (tri_delaunay2d)
    {
        throw runtime_error("ERROR: cannot load sample data twice!");
    }
    // load vertex data
    vector<del_point2d_t> points;
    PROFILE(readDataset(sampleDataPath, points));

    // triangulate vertx data
    delaunay2d_t *delaunay;
    PROFILE(delaunay = delaunay2d_from((del_point2d_t*)points.data(), points.size()));
    PROFILE(tri_delaunay2d = tri_delaunay2d_from(delaunay));
    delaunay2d_release(delaunay);

    // compute normals
    PROFILE(computeNormals());
}

void DataSample::readDataset(const string &filePath, vector<del_point2d_t> &points)
{
    // try open file
    FILE* datasetFile = fopen(filePath.c_str(), "r");
    if (!datasetFile)
        throw runtime_error("Unable to open file " + filePath);

    // min and max values for normalization
    float min_intensity = numeric_limits<float>::max();
    float max_intensity = numeric_limits<float>::min();

    // total point value for average
    Vector3f total_point = { 0.0f, 0.0f, 0.0f };

    // path segments must always contain the first point...
    m_PathSegments.push_back(0);

    unsigned int lineNumber = 0;
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
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
            m_Metadata.parse(head);
            if (m_Metadata.datapointsInFile >= 0)
            {
                // as soon as we know the total size of the dataset, reserve enough space for it
                points.reserve(m_Metadata.datapointsInFile);
                m_SelectedPoints.resize(m_Metadata.datapointsInFile, 0);
                m_Heights.reserve(m_Metadata.datapointsInFile);
                m_LogHeights.reserve(m_Metadata.datapointsInFile);

                m_RawPoints.reserve(m_Metadata.datapointsInFile);
                m_PointsInfo.pointCount = m_Metadata.datapointsInFile;
            }
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
            m_RawPoints.push_back(RawDataPoint{ theta, phi, intensity });
            del_point2d_t transformedPoint = m_RawPoints.back().transform();
            // if two last points are too far appart, a new path segments begins
            if (!points.empty())
            {
                const del_point2d_t& prev = points.back();
                float dx = transformedPoint.x - prev.x;
                float dz = transformedPoint.y - prev.y;
                if (dx*dx + dz*dz > MAX_SAMPLING_DISTANCE)
                {
                    m_PathSegments.push_back(points.size());
                }
            }

            points.push_back({ transformedPoint.x, transformedPoint.y });
            m_Heights.push_back(intensity);
            m_LogHeights.push_back(intensity);

            min_intensity = min(min_intensity, intensity);
            max_intensity = max(max_intensity, intensity);

            total_point += Vector3f{ transformedPoint.x, intensity, transformedPoint.y };
        }
    }
    fclose(datasetFile);
    
    // ...and the last one
    m_PathSegments.push_back(points.size());

    // normalize intensities
    float correction_factor = 0.0f;
    if (min_intensity <= 0.0f)
        correction_factor = -min_intensity + 1e-10f;
    float min_log_intensity = log(min_intensity + correction_factor);
    float max_log_intensity = log(max_intensity);
    for (size_t i = 0; i < m_Heights.size(); ++i)
    {
        m_Heights[i] = (m_Heights[i] - min_intensity) / (max_intensity - min_intensity);
        m_LogHeights[i] = (log(m_LogHeights[i] + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
    }
    m_PointsInfo.minMaxHeights = make_pair(min_intensity, max_intensity);
    m_PointsInfo.averagePoint = total_point / points.size();
    // normalize averagePoint intensity
    m_PointsInfo.averagePoint[1] = (m_PointsInfo.averagePoint[1] - min_intensity) / (max_intensity - min_intensity);

    m_Axis.setOrigin(m_PointsInfo.averagePoint);
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

    const string shader_path = "../resources/shaders/";
    m_Shaders[NORMAL].initFromFiles ("height_map",       shader_path + "height_map.vert",       shader_path + "height_map.frag");
    m_Shaders[LOG].initFromFiles    ("log_map",          shader_path + "height_map.vert",       shader_path + "height_map.frag");
    m_Shaders[PATH].initFromFiles   ("path",             shader_path + "path.vert",             shader_path + "path.frag");
    m_Shaders[POINTS].initFromFiles ("selected_points",  shader_path + "selected_points.vert",  shader_path + "selected_points.frag");

    m_Shaders[NORMAL].setUniform("color_map", 0);
    m_Shaders[LOG].setUniform("color_map", 0);

    m_Shaders[NORMAL].bind();
    m_Shaders[NORMAL].uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_Normals.data());
    m_Shaders[NORMAL].uploadAttrib("in_pos2d", tri_delaunay2d->num_points, 2, sizeof(del_point2d_t), GL_FLOAT, GL_FALSE, (const void*)tri_delaunay2d->points);
    m_Shaders[NORMAL].uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_Heights.data());
    m_Shaders[NORMAL].uploadAttrib("indices", tri_delaunay2d->num_triangles, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, tri_delaunay2d->tris);

    m_Shaders[LOG].bind();
    m_Shaders[LOG].uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_LogNormals.data());
    m_Shaders[LOG].shareAttrib(m_Shaders[NORMAL], "in_pos2d");
    m_Shaders[LOG].uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_LogHeights.data());
    m_Shaders[LOG].shareAttrib(m_Shaders[NORMAL], "indices");

    m_Shaders[PATH].bind();
    m_Shaders[PATH].shareAttrib(m_Shaders[NORMAL], "in_pos2d");
    m_Shaders[PATH].shareAttrib(m_Shaders[NORMAL], "in_height");

    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].shareAttrib(m_Shaders[NORMAL], "in_pos2d");
    m_Shaders[POINTS].shareAttrib(m_Shaders[NORMAL], "in_height");
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(unsigned char), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());

    m_Axis.loadShader();

    m_ShaderLinked = true;
}

void DataSample::selectPoints(const Matrix4f & mvp, const Vector2i & topLeft,
    const Vector2i & size, const Vector2i & canvasSize, SelectionMode mode)
{
    Vector3f total_point{ 0.0f, 0.0f, 0.0f };
    float min_intensity = numeric_limits<float>::max();
    float max_intensity = numeric_limits<float>::min();
    m_SelectedPointsInfo.pointCount = 0;
    for (unsigned int i = 0; i < tri_delaunay2d->num_points; ++i)
    {
        Vector3f point = getVertex(i, false);
        Vector4f homogeneousPoint;
        homogeneousPoint << point, 1.0f;
        Vector4f projPoint = mvp * homogeneousPoint;

        projPoint /= projPoint[3];
        projPoint[0] = (projPoint[0] + 1.0f) * 0.5f * canvasSize[0];
        projPoint[1] = canvasSize[1] - (projPoint[1] + 1.0f) * 0.5f * canvasSize[1];

        bool inSelection = projPoint[0] >= topLeft[0] && projPoint[0] <= topLeft[0] + size[0] &&
            projPoint[1] >= topLeft[1] && projPoint[1] <= topLeft[1] + size[1];
        
        switch (mode)
        {
        case NORMAL:
            m_SelectedPoints[i] = inSelection;
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
            total_point += getVertex(i, false);
            min_intensity = min(min_intensity, m_RawPoints[i].intensity);
            max_intensity = max(max_intensity, m_RawPoints[i].intensity);
            ++m_SelectedPointsInfo.pointCount;
        }
    }
    m_SelectedPointsInfo.averagePoint = total_point / m_SelectedPointsInfo.pointCount;
    m_SelectedPointsInfo.minMaxHeights = make_pair(min_intensity, max_intensity);

    // snap to selection
    if (m_SelectedPointsInfo.pointCount == 0)
        m_Axis.setOrigin(m_PointsInfo.averagePoint);
    else
        m_Axis.setOrigin(m_SelectedPointsInfo.averagePoint);

    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(unsigned char), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());
}

void DataSample::deselectAllPoints()
{
    memset(m_SelectedPoints.data(), 0, m_SelectedPoints.size() * sizeof(m_SelectedPoints[0]));
    m_Shaders[POINTS].bind();
    m_Shaders[POINTS].uploadAttrib("in_selected", m_SelectedPoints.size(), 1, sizeof(unsigned char), GL_BYTE, GL_FALSE, (const void*)m_SelectedPoints.data());

    m_SelectedPointsInfo.pointCount = 0;
}

nanogui::Vector3f DataSample::selectionCenter()
{
    return m_SelectedPointsInfo.pointCount == 0 ? m_PointsInfo.averagePoint : m_SelectedPointsInfo.averagePoint;
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
    m_Normals.resize(tri_delaunay2d->num_points, { 0,0,0 });
    m_LogNormals.resize(tri_delaunay2d->num_points, { 0,0,0 });

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