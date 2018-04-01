#pragma once

#include <nanogui/nanogui.h>
#include <vector>
#include "delaunay.h"

struct DataSample
{

public:
	DataSample();
	DataSample(const std::string& sampleDataPath);
	DataSample(const DataSample&) = delete;
	DataSample(DataSample&&) = default;

    ~DataSample();

	DataSample& operator=(const DataSample&) = delete;
	DataSample& operator=(DataSample&&) = default;

    void draw(  const nanogui::Matrix4f& model,
                const nanogui::Matrix4f& view,
                const nanogui::Matrix4f& proj);

    void displayNormalView(bool value)	{ m_DisplayNormalView = value; }
    void displayLogView(bool value)		{ m_DisplayLogView = value; }
    void displayPath(bool value)		{ m_DisplayPath = value; }

    void loadFromFile(const std::string& sampleDataPath);

private:
	inline nanogui::Vector3f getVertex(unsigned int i, bool logged) const;
	void computeTriangleNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged);

	void readDataset(const std::string &filePath, std::vector<del_point2d_t> &points);
	void computeNormals();
	void linkDataToShaders();

public:
	template<typename T>
	struct MetadataElem
	{
		std::string nameInFile;
		std::string name;
		T value;
	};
	struct Metadata
	{
		std::string mountainVersion;
		std::string databaseHost;
		std::string databaseName;
		std::string dumpHost;
		unsigned int databaseId;
		unsigned int datapointsInDatabase;

		std::string measuredAt;
		std::string dataReadFromDatabaseAt;

		std::string sampleLabel;
		std::string sampleName;
		std::string lamp;

		float incidentTheta;
		float incidentPhi;
		float frontIntegral;

		float mountainMinimumAngle;
		unsigned int datapointsInFile;

		std::string getInfos() const;
	};
	
	const Metadata& metadata() const { return m_Metadata; }

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

	// metadata
	Metadata m_Metadata;
};