#pragma once

#include <nanogui/nanogui.h>
#include <vector>

#include "sample_data_parser.h"

struct SampleData
{
public:
	struct MetaData
	{
		std::string mountainVersion;
		std::string databaseHost;
		std::string databaseName;
		std::string databaseId;

		std::string measuredAt;
		std::string dataReadFromDatabaseAt;

		std::string sampleLabel;
		std::string sampleName;
		std::string lamp;

		float incidentTheta;
		float incidentPhi;
		float frontIntegral;

		float moutainMinimumAngle;
		unsigned int datapointsInFile;
	};

public:
	SampleData();
	SampleData(const std::string& sampleDataPath);
    ~SampleData();

    void draw(  const nanogui::Matrix4f& model,
                const nanogui::Matrix4f& view,
                const nanogui::Matrix4f& proj);

    void displayNormalView(bool value) { m_DisplayNormalView = value; }
    void displayLogView(bool value) { m_DisplayLogView = value; }
    void displayPath(bool value) { m_DisplayPath = value; }

    bool loadFromFile(const std::string& sampleDataPath);

private:
	bool readDataset(const std::string &filePath, std::vector<del_point2d_t> &points);
	void linkDataToShaders();
	void computeNormals();

	inline nanogui::Vector3f getVertex(unsigned int i, bool logged) const
	{
		return { tri_delaunay2d->points[i].x, logged ? m_LogHeights[i] : m_Heights[i], tri_delaunay2d->points[i].y };
	}

	void computeNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged);

private:
	// Raw sample data
	tri_delaunay2d_t *tri_delaunay2d;
	std::vector<float>				m_Heights;
	std::vector<float>              m_LogHeights;
	std::vector<nanogui::Vector3f>  m_Normals;
	std::vector<nanogui::Vector3f>  m_LogNormals;

	// display Shaders
	nanogui::GLShader m_NormalShader;
	nanogui::GLShader m_LogShader;
	nanogui::GLShader m_PathShader;

	// display options
	bool m_DisplayNormalView;
	bool m_DisplayPath;
	bool m_DisplayLogView;
};