#include "RadialGrid.h"

using namespace nanogui;

RadialGrid::RadialGrid()
:	m_Color(200, 200, 200, 200)
,	m_Visible(true)
{
    m_Shader.initFromFiles("grid",
        "../resources/shaders/radial_grid.vert",
        "../resources/shaders/radial_grid.frag");

    std::vector<Vector3f> vertices(CIRCLE_COUNT * VERTEX_PER_CIRCLE_COUNT +
        LINE_COUNT * VERTEX_PER_LINE_COUNT);

    for (unsigned int i = 0; i < CIRCLE_COUNT; ++i)
    {
        float radius = (float)(i + 1) / CIRCLE_COUNT;
        for (unsigned int j = 0; j < VERTEX_PER_CIRCLE_COUNT; ++j)
        {
            Vector3f point{
                radius * (float)cos(2 * M_PI * j / VERTEX_PER_CIRCLE_COUNT), // x coord
                0,                                              // y coord
                radius * (float)sin(2 * M_PI * j / VERTEX_PER_CIRCLE_COUNT)  // z coord
            };
            vertices[i*VERTEX_PER_CIRCLE_COUNT + j] = point;
        }
    }

    for (unsigned int i = 0; i < LINE_COUNT; ++i)
    {
        unsigned int index = CIRCLE_COUNT * VERTEX_PER_CIRCLE_COUNT + i * VERTEX_PER_LINE_COUNT;
        double angle = M_PI * i / LINE_COUNT;
        vertices[index] = { (float)cos(angle), 0, (float)sin(angle) };
        vertices[index + 1] = -vertices[index];

        m_DegreesLabel.push_back(std::make_pair(std::to_string(180 * i / LINE_COUNT), vertices[index]));
        m_DegreesLabel.push_back(std::make_pair(std::to_string(180 * i / LINE_COUNT + 180), vertices[index+1]));
    }

    m_Shader.bind();
    m_Shader.uploadAttrib("in_pos", vertices.size(), 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)vertices.data());
}

RadialGrid::~RadialGrid()
{
    m_Shader.free();
}

void RadialGrid::drawGL(const Matrix4f& model,
    const Matrix4f& view,
    const Matrix4f& proj)
{
    if (m_Visible)
    {
        Matrix4f mvp = proj * view * model;
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_Shader.bind();
        m_Shader.setUniform("mvp", mvp);

        for (size_t i = 0; i < 2; i++)
        {
            glDepthFunc(i % 2 == 0 ? GL_LESS : GL_GREATER);
            m_Shader.setUniform("in_color", i % 2 == 0 ? m_Color : m_Color * 0.6);
            for (unsigned int i = 0; i < CIRCLE_COUNT; ++i)
            {
                m_Shader.drawArray(GL_LINE_LOOP, i*VERTEX_PER_CIRCLE_COUNT, VERTEX_PER_CIRCLE_COUNT);
            }
            m_Shader.drawArray(GL_LINES, CIRCLE_COUNT*VERTEX_PER_CIRCLE_COUNT, LINE_COUNT*VERTEX_PER_LINE_COUNT);
        }

        // Restore opengl settings
        glDepthFunc(GL_LESS);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
}

void RadialGrid::draw(  NVGcontext *ctx,
                        const Vector2i &canvasSize,
                        const Matrix4f &model,
                        const Matrix4f &view,
                        const Matrix4f &proj)
{
    if (m_Visible)
    {
        nvgFontSize(ctx, 15.0f);
        nvgFontFace(ctx, "sans");
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(ctx, Color(1.0f, 0.8f));
        for (const auto& degreeLabel : m_DegreesLabel)
        {
            Vector4f homogeneousPoint;
            homogeneousPoint << degreeLabel.second * 1.05f, 1.0f;
            Vector4f projectedPoint{ proj * view * model * homogeneousPoint };

            projectedPoint /= projectedPoint[3];
            projectedPoint[0] = (projectedPoint[0] + 1.0f) * 0.5f * canvasSize.x();
            projectedPoint[1] = canvasSize.y() - (projectedPoint[1] + 1.0f) * 0.5f * canvasSize.y();
            nvgText(ctx, projectedPoint[0], projectedPoint[1], degreeLabel.first.c_str(), nullptr);
        }
    }

}
