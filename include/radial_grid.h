#pragma once

#include <nanogui/nanogui.h>
#include <vector>

class RadialGrid
{
public:
    RadialGrid()
    :   m_CircleCount(5)
    ,   m_VertexPerCircleCount(50)
    ,   m_LinesCount(10)
    ,   m_VertexPerLineCount(2)
    ,   m_Color(200, 200, 200, 200)
    ,   m_Visible(true)
    {
        m_Shader.initFromFiles("grid", 
        "../resources/shaders/radial_grid.vert",
        "../resources/shaders/radial_grid.frag");

        std::vector<nanogui::Vector3f> vertices(m_CircleCount * m_VertexPerCircleCount +
                                                m_LinesCount * m_VertexPerLineCount);

        for (unsigned int i = 0; i < m_CircleCount; ++i)
        {
            float radius = (i+1) / m_CircleCount;
            for (unsigned int j = 0; j < m_VertexPerCircleCount; ++j)
            {
                vertices[i*m_VertexPerCircleCount + j] = {
                    radius * (float)cos(2 * M_PI * j / m_VertexPerCircleCount), // x coord
                    0,                                              // y coord
                    radius * (float)sin(2 * M_PI * j / m_VertexPerCircleCount)  // z coord
                };
            }
        }

        for (unsigned int i = 0; i < m_LinesCount; ++i)
        {
            unsigned int index = m_CircleCount * m_VertexPerCircleCount + i * m_VertexPerLineCount;
            vertices[index]     = {(float)cos(M_PI * i / m_LinesCount), 0, (float)sin(M_PI * i / m_LinesCount)};
            vertices[index+1]   = {(float)cos(M_PI * i / m_LinesCount + M_PI), 0, (float)sin(M_PI * i / m_LinesCount + M_PI)};
        }

        m_Shader.bind();
        m_Shader.uploadAttrib("in_pos", vertices.size(), 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)vertices.data());
    }
    ~RadialGrid()
    {
        m_Shader.free();
    }

    void draw(  const nanogui::Matrix4f& model,
                const nanogui::Matrix4f& view,
                const nanogui::Matrix4f& proj)
    {
        if (m_Visible)
        {
            nanogui::Matrix4f mvp = proj * view * model;
            glEnable(GL_DEPTH_TEST);
            glPointSize(2);
            m_Shader.bind();
            m_Shader.setUniform("mvp", mvp);
            m_Shader.setUniform("in_color", m_Color);
            for (unsigned int i = 0; i < m_CircleCount; ++i)
            {
                m_Shader.drawArray(GL_LINE_LOOP, i*m_VertexPerCircleCount, m_VertexPerCircleCount);
            }
            m_Shader.drawArray(GL_LINES, m_CircleCount*m_VertexPerCircleCount, m_LinesCount*m_VertexPerLineCount);
            glDisable(GL_DEPTH_TEST);
        }
    }

    void setVisible(bool visible) { m_Visible = visible; }
    bool visible() const { return m_Visible; }

    const nanogui::Color& color() const { return m_Color; }
    void setColor(const nanogui::Color& newColor) { m_Color = newColor; }
    
private:
    nanogui::GLShader m_Shader;
    unsigned int m_CircleCount;
    unsigned int m_VertexPerCircleCount;
    unsigned int m_LinesCount;
    unsigned int m_VertexPerLineCount;
    nanogui::Color m_Color;

    bool m_Visible;
};