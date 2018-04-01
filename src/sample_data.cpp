#include "sample_data.h"

#include <cstdint>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <delaunay.h>

#include "common.h"
#include "stop_watch.h"

SampleData::SampleData()
:	m_DisplayNormalView(true)
,	m_DisplayPath(false)
,	m_DisplayLogView(false)
,	tri_delaunay2d(nullptr)
{
	m_NormalShader.initFromFiles(
		"height_map",
		"../resources/shaders/height_map.vert",
		"../resources/shaders/height_map.frag"
	);
	m_LogShader.initFromFiles(
		"log_map",
		"../resources/shaders/height_map.vert",
		"../resources/shaders/height_map.frag"
	);
	m_PathShader.initFromFiles(
		"path_drawer",
		"../resources/shaders/path.vert",
		"../resources/shaders/path.frag"
	);
}

SampleData::SampleData(const std::string& sampleDataPath)
:	SampleData()
{
    loadFromFile(sampleDataPath);
}

SampleData::~SampleData()
{
    m_NormalShader.free();
	m_LogShader.free();
	m_PathShader.free();

	tri_delaunay2d_release(tri_delaunay2d);
}

void SampleData::draw( 
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
    
		if (m_DisplayNormalView)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			m_NormalShader.bind();
			m_NormalShader.setUniform("modelViewProj", mvp);
			m_NormalShader.setUniform("model", model);
			m_NormalShader.setUniform("view", Vector3f(0, 0, 4));
			m_NormalShader.drawIndexed(GL_TRIANGLES, 0, tri_delaunay2d->num_triangles);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		if (m_DisplayLogView)
		{
			m_LogShader.bind();
			m_LogShader.setUniform("modelViewProj", mvp);
			m_LogShader.setUniform("model", model);
			m_LogShader.setUniform("view", Vector3f(0, 0, 4));
			m_LogShader.drawIndexed(GL_TRIANGLES, 0, tri_delaunay2d->num_triangles);
		}
		if (m_DisplayPath)
		{
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glPolygonOffset(-2.0, -2.0);
			m_PathShader.bind();
			m_PathShader.setUniform("modelViewProj", mvp);
			m_PathShader.drawArray(GL_LINE_STRIP, 0, tri_delaunay2d->num_points);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDisable(GL_POLYGON_OFFSET_LINE);
		}
		glDisable(GL_DEPTH_TEST);
	}
}

bool SampleData::loadFromFile(const std::string& sampleDataPath)
{
	if (tri_delaunay2d)
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

	linkDataToShaders();
}

void SampleData::linkDataToShaders()
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

bool SampleData::readDataset(const std::string &filePath, std::vector<del_point2d_t> &points)
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
	{
	}

	while (fscanf(datasetFile, "%f %f %f", &theta, &phi, &intensity) == 3)
	{
		float x = theta * cos(phi * M_PI / 180.0f) / 90;
		float z = theta * sin(phi * M_PI / 180.0f) / 90;

		points.push_back({ x, z });
		m_Heights.push_back(intensity);
		m_LogHeights.push_back(intensity);

		min_intensity = std::min(min_intensity, intensity);
		max_intensity = std::max(max_intensity, intensity);
	}
	fclose(datasetFile);

	// intensity normalization
	float min_log_intensity = log(min_intensity + 1);
	float max_log_intensity = log(max_intensity + 1);
	for (size_t i = 0; i < m_Heights.size(); ++i)
	{
		m_Heights[i] = (m_Heights[i] - min_intensity) / (max_intensity - min_intensity);
		m_LogHeights[i] = (log(m_LogHeights[i] + 1) - min_log_intensity) / (max_log_intensity - min_log_intensity);
	}

	return true;
}

void SampleData::computeNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged)
{
	std::vector<nanogui::Vector3f> &normals = logged ? m_LogNormals : m_Normals;

	nanogui::Vector3f faceNormal =	(getVertex(i2, logged) - getVertex(i0, logged)).cross
									(getVertex(i1, logged) - getVertex(i0, logged)).normalized();

	const nanogui::Vector3f e01 = (getVertex(i1, logged) - getVertex(i0, logged)).normalized();
	const nanogui::Vector3f e12 = (getVertex(i2, logged) - getVertex(i1, logged)).normalized();
	const nanogui::Vector3f e20 = (getVertex(i0, logged) - getVertex(i2, logged)).normalized();
	float w0 = (float)acos(std::max(-1.0f, std::min(1.0f, e01.dot(-e20))));
	float w1 = (float)acos(std::max(-1.0f, std::min(1.0f, e12.dot(-e01))));
	float w2 = (float)acos(std::max(-1.0f, std::min(1.0f, e20.dot(-e12))));

	normals[i0] += w0 * faceNormal;
	normals[i1] += w1 * faceNormal;
	normals[i2] += w2 * faceNormal;
}

void SampleData::computeNormals()
{
	m_Normals.resize(tri_delaunay2d->num_triangles * 3, { 0,0,0 });
	m_LogNormals.resize(tri_delaunay2d->num_triangles * 3, { 0,0,0 });

	for (unsigned int i = 0; i < tri_delaunay2d->num_triangles; i += 3)
	{
		const unsigned int &i0 = tri_delaunay2d->tris[3 * i];
		const unsigned int &i1 = tri_delaunay2d->tris[3 * i + 1];
		const unsigned int &i2 = tri_delaunay2d->tris[3 * i + 2];

		computeNormal(i0, i1, i2, false);
		computeNormal(i0, i1, i2, true);
	}

	for (size_t i = 0; i < m_Normals.size(); ++i)
	{
		m_Normals[i].normalize();
		m_LogNormals[i].normalize();
	}
}