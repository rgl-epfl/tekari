#include "sample_data.h"

SampleData::SampleData(const std::string& sampleDataPath)
:   m_DisplayNormalView(true)
,   m_DisplayPath(false)
,   m_DisplayLogView(false)
{   
    m_NormalShader.initFromFiles(
        "height_map",
        "../resources/shaders/height_map.vert",
        "../resources/shaders/height_map.frag"
    );
    m_LogShader.initFromFiles(
        "height_map",
        "../resources/shaders/height_map.vert",
        "../resources/shaders/height_map.frag"
    );
    m_PathShader.initFromFiles(
        "path_drawer",
        "../resources/shaders/path.vert",
        "../resources/shaders/path.frag"
    );
    loadFromFile(sampleDataPath);
}

SampleData::~SampleData()
{
    m_NormalShader.free();
    m_LogShader.free();
}

void SampleData::loadFromFile(const std::string& sampleDataPath)
{
    SampleDataParser os(sampleDataPath);
    m_NFaces = os.getNFaces();

    os.linkDataToShaders(m_NormalShader, m_LogShader, m_PathShader);
}


void SampleData::draw( 
    const nanogui::Matrix4f& model,
    const nanogui::Matrix4f& view,
    const nanogui::Matrix4f& proj)
{
    using namespace nanogui;

    Matrix4f mvp = proj * view * model;

    glEnable(GL_DEPTH_TEST);
    glPointSize(2);
    
    if (m_DisplayNormalView)
    {
        m_NormalShader.bind();
        m_NormalShader.setUniform("modelViewProj", mvp);
        m_NormalShader.setUniform("model", model);
        m_NormalShader.setUniform("view", Vector3f(0, 0, 4));
        m_NormalShader.drawIndexed(GL_TRIANGLES, 0, m_NFaces);
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
        m_PathShader.bind();
        m_PathShader.setUniform("modelViewProj", mvp);
        m_PathShader.drawIndexed(GL_POINTS, 0, m_NFaces);    
    }
    glDisable(GL_DEPTH_TEST);
}