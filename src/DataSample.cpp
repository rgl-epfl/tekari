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

#define MAX_SAMPLING_DISTANCE 0.05f

DataSample::DataSample(std::shared_ptr<ColorMap> colorMap)
:	m_DisplayViews{ true, false, false }
,	tri_delaunay2d(nullptr)
,	m_ColorMap(colorMap)
{
    m_NormalShader.initFromFiles(
        "height_map",
        "../resources/shaders/height_map.vert",
        "../resources/shaders/height_map.frag"
    );
    m_NormalShader.setUniform("color_map", 0);
    m_LogShader.initFromFiles(
        "log_map",
        "../resources/shaders/height_map.vert",
        "../resources/shaders/height_map.frag"
    );
    m_LogShader.setUniform("color_map", 0);

    m_PathShader.initFromFiles(
        "path_drawer",
        "../resources/shaders/path.vert",
        "../resources/shaders/path.frag"
    );
}

DataSample::DataSample(std::shared_ptr<ColorMap> colorMap, const std::string& sampleDataPath)
:	DataSample(colorMap)
{
    loadFromFile(sampleDataPath);
}

DataSample::~DataSample()
{
    m_NormalShader.free();
    m_LogShader.free();
    m_PathShader.free();

    if (tri_delaunay2d)
    {
        tri_delaunay2d_release(tri_delaunay2d);
    }
}

void DataSample::drawGL(
    const nanogui::Vector3f& viewOrigin,
    const nanogui::Matrix4f& model,
    const nanogui::Matrix4f& view,
    const nanogui::Matrix4f& proj)
{
    using namespace nanogui;
    if (tri_delaunay2d)
    {
        Matrix4f mvp = proj * view * model;

        glEnable(GL_DEPTH_TEST);
        glPointSize(2);
    
        if (m_DisplayViews[NORMAL])
        {
            m_NormalShader.bind();
            m_ColorMap->bind();
            m_NormalShader.setUniform("modelViewProj", mvp);
            m_NormalShader.setUniform("model", model);
            m_NormalShader.setUniform("view", viewOrigin);
            m_NormalShader.drawIndexed(GL_TRIANGLES, 0, tri_delaunay2d->num_triangles);
        }
        if (m_DisplayViews[LOG])
        {
            m_LogShader.bind();
            m_ColorMap->bind();
            m_LogShader.setUniform("modelViewProj", mvp);
            m_LogShader.setUniform("model", model);
            m_LogShader.setUniform("view", viewOrigin);
            m_LogShader.drawIndexed(GL_TRIANGLES, 0, tri_delaunay2d->num_triangles);
        }
        if (m_DisplayViews[PATH])
        {
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glPolygonOffset(2.0, 2.0);
            m_PathShader.bind();
            m_PathShader.setUniform("modelViewProj", mvp);
            for (unsigned int i = 0; i < m_PathSegments.size()-1; ++i)
            {
                int offset = m_PathSegments[i];
                int count = m_PathSegments[i+1] - m_PathSegments[i] - 1;
                m_PathShader.drawArray(GL_LINE_STRIP, offset, count);
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }
        glDisable(GL_DEPTH_TEST);
    }
}

void DataSample::loadFromFile(const std::string& sampleDataPath)
{
    if (tri_delaunay2d)
    {
        throw std::runtime_error("ERROR: cannot load sample data twice!");
    }
    // load vertex data
    std::vector<del_point2d_t> points;
    PROFILE(readDataset(sampleDataPath, points));

    // triangulate vertx data
    delaunay2d_t *delaunay;
    PROFILE(delaunay = delaunay2d_from((del_point2d_t*)points.data(), points.size()));
    PROFILE(tri_delaunay2d = tri_delaunay2d_from(delaunay));
    delaunay2d_release(delaunay);

    // compute normals
    PROFILE(computeNormals());

    linkDataToShaders();
}

void DataSample::readDataset(const std::string &filePath, std::vector<del_point2d_t> &points)
{
    // try open file
    FILE* datasetFile = fopen(filePath.c_str(), "r");
    if (!datasetFile)
        throw std::runtime_error("Unable to open file " +filePath);

    // min and max values for normalization
    float min_intensity = std::numeric_limits<float>::max();
    float max_intensity = std::numeric_limits<float>::min();

    // path segments must always contain the first point...
    m_PathSegments.push_back(0);

    unsigned int lineNumber = 0;
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
    while (!feof(datasetFile) && !ferror(datasetFile) && fgets(line, MAX_LENGTH, datasetFile))
    {
        ++lineNumber;
        if (strlen(line) <= 1)
        {
            // skip empty lines
        }
        else if (line[0] == '#')
        {
            m_Metadata.parse(line);
            if (m_Metadata.datapointsInFile >= 0)
            {
                // as soon as we know the total size of the dataset, reserve enough space for it
                points.reserve(m_Metadata.datapointsInFile);
                m_Heights.reserve(m_Metadata.datapointsInFile);
                m_LogHeights.reserve(m_Metadata.datapointsInFile);
            }
        }
        else
        {
            float phi, theta, intensity;
            if (sscanf(line, "%f %f %f", &theta, &phi, &intensity) != 3)
            {
                fclose(datasetFile);
                std::ostringstream errorMsg;
                errorMsg << "Invalid file format: " << line << " (line " << lineNumber << ")";
                throw std::runtime_error(errorMsg.str());
            }

            float x = theta * cos(phi * M_PI / 180.0f) / 90.0f;
            float z = theta * sin(phi * M_PI / 180.0f) / 90.0f;

            // if two last points are too far appart, a new path segments begins
            if (!points.empty())
            {
                const del_point2d_t& prev = points.back();
                if ((x - prev.x)*(x - prev.x) + (z - prev.y)*(z - prev.y) > MAX_SAMPLING_DISTANCE)
                {
                    m_PathSegments.push_back(points.size());
                }
            }

            points.push_back({ x, z });
            m_Heights.push_back(intensity);
            m_LogHeights.push_back(intensity);

            min_intensity = std::min(min_intensity, intensity);
            max_intensity = std::max(max_intensity, intensity);
        }
    }
    fclose(datasetFile);
    
    // ...and the last one
    m_PathSegments.push_back(points.size());

    // normalize intensities
    float min_log_intensity = log(min_intensity + 1);
    float max_log_intensity = log(max_intensity + 1);
    for (size_t i = 0; i < m_Heights.size(); ++i)
    {
        m_Heights[i] = (m_Heights[i] - min_intensity) / (max_intensity - min_intensity);
        m_LogHeights[i] = (log(m_LogHeights[i] + 1) - min_log_intensity) / (max_log_intensity - min_log_intensity);
    }
}

void DataSample::linkDataToShaders()
{
    if (!tri_delaunay2d)
    {
        throw std::runtime_error("ERROR: cannot link data to shader before loading.");
    }
    m_NormalShader.bind();
    m_NormalShader.uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_Normals.data());
    m_NormalShader.uploadAttrib("in_pos2d", tri_delaunay2d->num_points, 2, sizeof(del_point2d_t), GL_FLOAT, GL_FALSE, (const void*)tri_delaunay2d->points);
    m_NormalShader.uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_Heights.data());
    m_NormalShader.uploadAttrib("indices", tri_delaunay2d->num_triangles, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, tri_delaunay2d->tris);

    m_LogShader.bind();
    m_LogShader.uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_LogNormals.data());
    m_LogShader.shareAttrib(m_NormalShader, "in_pos2d");
    m_LogShader.uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_LogHeights.data());
    m_LogShader.shareAttrib(m_NormalShader, "indices");

    m_PathShader.bind();
    m_PathShader.shareAttrib(m_NormalShader, "in_pos2d");
    m_PathShader.shareAttrib(m_NormalShader, "in_height");
    m_PathShader.shareAttrib(m_NormalShader, "indices");
}

nanogui::Vector3f DataSample::getVertex(unsigned int i, bool logged) const
{
    return {	tri_delaunay2d->points[i].x,
                logged ? m_LogHeights[i] : m_Heights[i],
                tri_delaunay2d->points[i].y };
}

void DataSample::computeTriangleNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged)
{
    const nanogui::Vector3f e01 = (getVertex(i1, logged) - getVertex(i0, logged)).normalized();
    const nanogui::Vector3f e12 = (getVertex(i2, logged) - getVertex(i1, logged)).normalized();
    const nanogui::Vector3f e20 = (getVertex(i0, logged) - getVertex(i2, logged)).normalized();

    nanogui::Vector3f faceNormal =	-e20.cross(e01).normalized();

    float w0 = (float)acos(std::max(-1.0f, std::min(1.0f, e01.dot(-e20))));
    float w1 = (float)acos(std::max(-1.0f, std::min(1.0f, e12.dot(-e01))));
    float w2 = (float)acos(std::max(-1.0f, std::min(1.0f, e20.dot(-e12))));

    std::vector<nanogui::Vector3f> &normals = logged ? m_LogNormals : m_Normals;
    normals[i0] += w0 * faceNormal;
    normals[i1] += w1 * faceNormal;
    normals[i2] += w2 * faceNormal;
}

void DataSample::computeNormals()
{
    m_Normals.resize(tri_delaunay2d->num_triangles * 3, { 0,0,0 });
    m_LogNormals.resize(tri_delaunay2d->num_triangles * 3, { 0,0,0 });

    for (unsigned int i = 0; i < tri_delaunay2d->num_triangles; i += 3)
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