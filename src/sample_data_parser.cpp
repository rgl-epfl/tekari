#include "sample_data_parser.h"

#include <cstdint>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <libqhullcpp/Qhull.h>
#include <iostream>

#include "common.h"
#include "stop_watch.h"

#include <delaunay.h>

SampleDataParser::SampleDataParser(const std::string& sampleDataPath)
:   SampleDataParser()
{
    loadFromFile(sampleDataPath);
}

SampleDataParser::SampleDataParser(const SampleDataParser && other)
:	tri_delaunay2d(other.tri_delaunay2d)
,	m_Heights(std::move(other.m_Heights))
,	m_LogHeights(std::move(other.m_LogHeights))
,	m_Normals(std::move(other.m_Normals))
,	m_LogNormals(std::move(other.m_LogNormals))
,	m_AlreadyLoaded(other.m_AlreadyLoaded)
{}

SampleDataParser::SampleDataParser()
:   tri_delaunay2d(nullptr)
,	m_AlreadyLoaded(false)
{}

bool SampleDataParser::loadFromFile(const std::string& sampleDataPath)
{
    if (m_AlreadyLoaded)
    {
        throw std::runtime_error("ERROR: cannot load sample data twice!");
    }
    double readDataElapsed;
    double runDelaunayElapsed;
	double triangulateDelaunayElapsed;
	double computeNormalsElapsed;
	
    // load vertex data
	std::vector<del_point2d_t> points;
    PROFILE(readDataset(sampleDataPath, points), readDataElapsed);

	delaunay2d_t *delaunay;
	PROFILE(delaunay = delaunay2d_from((del_point2d_t*)points.data(), points.size()), runDelaunayElapsed);
	PROFILE(tri_delaunay2d = tri_delaunay2d_from(delaunay), triangulateDelaunayElapsed);
	delaunay2d_release(delaunay);
	PROFILE(computeNormals(), computeNormalsElapsed);

    double totalElapsed = readDataElapsed + runDelaunayElapsed + triangulateDelaunayElapsed + computeNormalsElapsed;
    std::cout << "Read data percentage " << (100 * readDataElapsed / totalElapsed) << "%" << std::endl;
    std::cout << "Run delaunay percentage " << (100 * runDelaunayElapsed / totalElapsed) << "%" << std::endl;
	std::cout << "Triangulate delaunay percentage " << (100 * triangulateDelaunayElapsed / totalElapsed) << "%" << std::endl;
	std::cout << "Compute normals percentage " << (100 * computeNormalsElapsed / totalElapsed) << "%" << std::endl;
	
    m_AlreadyLoaded = true;
}

void SampleDataParser::linkDataToShaders(   nanogui::GLShader &normalShader,
                                            nanogui::GLShader &logShader,
                                            nanogui::GLShader &pathShader)
{
    if (!m_AlreadyLoaded)
    {
        throw std::runtime_error("ERROR: cannot link data to shader before loading.");
    }
    normalShader.bind();
    normalShader.uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_Normals.data());
    normalShader.uploadAttrib("in_pos2d", tri_delaunay2d->num_points, 2, sizeof(del_point2d_t), GL_FLOAT, GL_FALSE, (const void*)tri_delaunay2d->points);
    normalShader.uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_Heights.data());
    normalShader.uploadAttrib("indices", tri_delaunay2d->num_triangles, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, tri_delaunay2d->tris);

    logShader.bind();
    logShader.uploadAttrib("in_normal", tri_delaunay2d->num_points, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_LogNormals.data());
    logShader.shareAttrib(normalShader, "in_pos2d");
    logShader.uploadAttrib("in_height", tri_delaunay2d->num_points, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_LogHeights.data());
    logShader.shareAttrib(normalShader, "indices");

    pathShader.bind();
    pathShader.shareAttrib(normalShader, "in_pos2d");
    pathShader.shareAttrib(normalShader, "in_height");
    pathShader.shareAttrib(normalShader, "indices");

	tri_delaunay2d_release(tri_delaunay2d);
}

bool SampleDataParser::readDataset(const std::string &filePath, std::vector<del_point2d_t> &points)
{
    // read file
    float phi, theta, intensity;

    FILE* datasetFile = fopen(filePath.c_str(), "r");
    if (!datasetFile)
        return false;

    // min and max values for normalization
    float min_intensity = std::numeric_limits<float>::max();
    float max_intensity = std::numeric_limits<float>::min();

    // TODO change this to some smarter stuff
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
    while (fgets(line, MAX_LENGTH, datasetFile) && line[0] == '#')
        {}

    while (fscanf(datasetFile, "%f %f %f", &theta, &phi, &intensity) == 3)
    {
        float x = theta * cos(phi * M_PI / 180.0f) / 90;
        float z = theta * sin(phi * M_PI / 180.0f) / 90;

        points.push_back({x, z});
        m_Heights.push_back(intensity);
        m_LogHeights.push_back(intensity);

        min_intensity = std::min(min_intensity, intensity);
        max_intensity = std::max(max_intensity, intensity);
    }
    fclose(datasetFile);

    // intensity normalization
    float min_log_intensity = log(min_intensity + 1);
    float max_log_intensity = log(max_intensity + 1);
    for(size_t i = 0; i < m_Heights.size(); ++i)
    {
        m_Heights[i]    = (m_Heights[i] - min_intensity) / (max_intensity - min_intensity);
        m_LogHeights[i] = (log(m_LogHeights[i] + 1) - min_log_intensity) / (max_log_intensity - min_log_intensity);
    }

    return true;
}

void SampleDataParser::angleWeights(unsigned int i0, unsigned int i1, unsigned int i2,
	float &w0, float &w1, float &w2) const {
	// compute angle weights
	const nanogui::Vector3f e01 = (getVertex(i1) - getVertex(i0)).normalized();
	const nanogui::Vector3f e12 = (getVertex(i2) - getVertex(i1)).normalized();
	const nanogui::Vector3f e20 = (getVertex(i0) - getVertex(i2)).normalized();
	w0 = (float)acos(std::max(-1.0f, std::min(1.0f, e01.dot(-e20))));
	w1 = (float)acos(std::max(-1.0f, std::min(1.0f, e12.dot(-e01))));
	w2 = (float)acos(std::max(-1.0f, std::min(1.0f, e20.dot(-e12))));
}
void SampleDataParser::angleLogWeights(unsigned int i0, unsigned int i1, unsigned int i2,
	float &w0, float &w1, float &w2) const {
	// compute angle weights
	const nanogui::Vector3f e01 = (getLogVertex(i1) - getLogVertex(i0)).normalized();
	const nanogui::Vector3f e12 = (getLogVertex(i2) - getLogVertex(i1)).normalized();
	const nanogui::Vector3f e20 = (getLogVertex(i0) - getLogVertex(i2)).normalized();
	w0 = (float)acos(std::max(-1.0f, std::min(1.0f, e01.dot(-e20))));
	w1 = (float)acos(std::max(-1.0f, std::min(1.0f, e12.dot(-e01))));
	w2 = (float)acos(std::max(-1.0f, std::min(1.0f, e20.dot(-e12))));
}

nanogui::Vector3f SampleDataParser::computeNormal(unsigned int i0, unsigned int i1, unsigned int i2) const
{
	nanogui::Vector3f e01 = (getVertex(i1) - getVertex(i0));
	nanogui::Vector3f e12 = (getVertex(i2) - getVertex(i1));
	nanogui::Vector3f e20 = (getVertex(i0) - getVertex(i2));
	return ((-e20).cross(e01).normalized() +
			(e20).cross(-e12).normalized() +
			(e12).cross(-e01).normalized()).normalized();
}

nanogui::Vector3f SampleDataParser::computeLogNormal(unsigned int i0, unsigned int i1, unsigned int i2) const
{
	return (getLogVertex(i2) - getLogVertex(i0)).cross(getLogVertex(i1) - getLogVertex(i0)).normalized();
}

void SampleDataParser::computeNormals()
{
	m_Normals.resize(tri_delaunay2d->num_triangles * 3, { 0,0,0 });
	m_LogNormals.resize(tri_delaunay2d->num_triangles * 3, { 0,0,0 });

	for (unsigned int i = 0; i < tri_delaunay2d->num_triangles; i += 3)
	{
		const unsigned int &i0 = tri_delaunay2d->tris[3*i];
		const unsigned int &i1 = tri_delaunay2d->tris[3*i+1];
		const unsigned int &i2 = tri_delaunay2d->tris[3*i+2];

		nanogui::Vector3f faceNormal = computeNormal(i0, i1, i2);
		nanogui::Vector3f logFaceNormal = computeLogNormal(i0, i1, i2);
		float w0, w1, w2, lw0, lw1, lw2;
		angleWeights(i0, i1, i2, w0, w1, w2);
		angleLogWeights(i0, i1, i2, lw0, lw1, lw2);

		m_Normals[i0] += w0 * faceNormal;
		m_Normals[i1] += w1 * faceNormal;
		m_Normals[i2] += w2 * faceNormal;

		m_LogNormals[i0] += lw0 * logFaceNormal;
		m_LogNormals[i1] += lw1 * logFaceNormal;
		m_LogNormals[i2] += lw2 * logFaceNormal;
	}

	for (size_t i = 0; i < m_Normals.size(); ++i)
	{
		m_Normals[i].normalize();
		m_LogNormals[i].normalize();
	}
}