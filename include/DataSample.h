#pragma once

#include <nanogui/common.h>
#include <nanogui/glutil.h>
#include <vector>
#include <memory>
#include "delaunay.h"
#include "Metadata.h"
#include "ColorMap.h"

struct DataSample
{
	enum Views
	{
		NORMAL = 0,
		LOG,
		PATH,
		VIEW_COUNT
	};

public:
	DataSample(std::shared_ptr<ColorMap> colorMap);
	DataSample(std::shared_ptr<ColorMap> colorMap, const std::string& sampleDataPath);
	DataSample(const DataSample&) = delete;
	DataSample(DataSample&&) = default;

    ~DataSample();

	DataSample& operator=(const DataSample&) = delete;
	DataSample& operator=(DataSample&&) = default;

    void draw(  const nanogui::Matrix4f& model,
                const nanogui::Matrix4f& view,
                const nanogui::Matrix4f& proj);

	void toggleView(Views view) { m_DisplayViews[view] = !m_DisplayViews[view]; }
	bool displayView(Views view) const { return m_DisplayViews[view]; }

	void setColorMap(std::shared_ptr<ColorMap> colorMap) { m_ColorMap = colorMap; }

    void loadFromFile(const std::string& sampleDataPath);

private:
	inline nanogui::Vector3f getVertex(unsigned int i, bool logged) const;
	void computeTriangleNormal(unsigned int i0, unsigned int i1, unsigned int i2, bool logged);

	void readDataset(const std::string &filePath, std::vector<del_point2d_t> &points);
	void computeNormals();
	void linkDataToShaders();

public:	
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
	std::shared_ptr<ColorMap> m_ColorMap;
	std::vector<unsigned int> m_PathSegments;

	// display options
	bool m_DisplayViews[VIEW_COUNT];

	// metadata
	Metadata m_Metadata;
};