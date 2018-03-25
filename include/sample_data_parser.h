#pragma once

#include <vector>
#include <nanogui/nanogui.h>
#include <iomanip>
#include <fstream>
#include <string>

#include "delaunay.h"

class SampleDataParser
{
    
public:
    SampleDataParser();
    SampleDataParser(const std::string& sampleDataPath);

    SampleDataParser(const SampleDataParser&) = delete;
    SampleDataParser(const SampleDataParser&& other);

    const SampleDataParser& operator=(const SampleDataParser&) = delete;
    const SampleDataParser& operator=(const SampleDataParser&& other) = delete;

    bool loadFromFile(const std::string& sampleDataPath);
    void linkDataToShaders( nanogui::GLShader &normalShader,
                            nanogui::GLShader &logShader,
                            nanogui::GLShader &pathShader);

	unsigned int getNFaces() const { return tri_delaunay2d->num_triangles; }
	unsigned int getNPoints() const { return tri_delaunay2d->num_points; }

private:
    bool readDataset(const std::string &filePath, std::vector<del_point2d_t> &points);
	void computeNormals();

	void angleWeights(unsigned int i0, unsigned int i1, unsigned int i2,
		float &w0, float &w1, float &w2) const;
	void angleLogWeights(unsigned int i0, unsigned int i1, unsigned int i2,
		float &w0, float &w1, float &w2) const;

    inline nanogui::Vector3f getVertex(unsigned int i) const { return { tri_delaunay2d->points[i].x, m_Heights[i], tri_delaunay2d->points[i].y }; }
    inline nanogui::Vector3f getLogVertex(unsigned int i) const { return { tri_delaunay2d->points[i].x, m_LogHeights[i], tri_delaunay2d->points[i].y }; }

	nanogui::Vector3f computeNormal(unsigned int i0, unsigned int i1, unsigned int i2) const;
	nanogui::Vector3f computeLogNormal(unsigned int i0, unsigned int i1, unsigned int i2) const;

private:
	tri_delaunay2d_t *tri_delaunay2d;

    std::vector<float>				m_Heights;
    std::vector<float>              m_LogHeights;
    std::vector<nanogui::Vector3f>  m_Normals;
    std::vector<nanogui::Vector3f>  m_LogNormals;

    bool m_AlreadyLoaded;
};