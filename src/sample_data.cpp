#include "sample_data.h"

SampleData::SampleData()
:	m_DisplayNormalView(true)
,	m_DisplayPath(false)
,	m_DisplayLogView(false)
,	m_Initialized(false)
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
}

void SampleData::loadFromFile(const std::string& sampleDataPath)
{
    SampleDataParser os(sampleDataPath);
    m_NFaces = os.getNFaces();
	m_NSamplePoints = os.getNPoints();

    os.linkDataToShaders(m_NormalShader, m_LogShader, m_PathShader);
	m_Initialized = true;
}


void SampleData::draw( 
    const nanogui::Matrix4f& model,
    const nanogui::Matrix4f& view,
    const nanogui::Matrix4f& proj)
{
    using namespace nanogui;
	if (m_Initialized)
	{
		Matrix4f mvp = proj * view * model;

		glEnable(GL_DEPTH_TEST);
		glPointSize(2);
    
		if (m_DisplayNormalView)
		{
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			m_NormalShader.bind();
			m_NormalShader.setUniform("modelViewProj", mvp);
			m_NormalShader.setUniform("model", model);
			m_NormalShader.setUniform("view", Vector3f(0, 0, 4));
			m_NormalShader.drawIndexed(GL_TRIANGLES, 0, m_NFaces);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		if (m_DisplayLogView)
		{
			m_LogShader.bind();
			m_LogShader.setUniform("modelViewProj", mvp);
			m_LogShader.setUniform("model", model);
			m_LogShader.setUniform("view", Vector3f(0, 0, 4));
			m_LogShader.drawIndexed(GL_TRIANGLES, 0, m_NFaces);    
		}
		if (m_DisplayPath)
		{
			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glPolygonOffset(-2.0, -2.0);
			m_PathShader.bind();
			m_PathShader.setUniform("modelViewProj", mvp);
			m_PathShader.drawArray(GL_LINE_STRIP, 0, m_NSamplePoints);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDisable(GL_POLYGON_OFFSET_LINE);
		}
		glDisable(GL_DEPTH_TEST);
	}
}