#include "tekari/RadialGrid.h"

using namespace nanogui;
using namespace std;

TEKARI_NAMESPACE_BEGIN

RadialGrid::RadialGrid()
:	mColor(200, 200, 200, 200)
,	mVisible(true)
,   mShowDegrees(true)
{
    mShader.initFromFiles("grid",
        "../resources/shaders/radial_grid.vert",
        "../resources/shaders/radial_grid.frag");

    vector<Vector3f> vertices(CIRCLE_COUNT * VERTEX_PER_CIRCLE_COUNT +
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
        int theta = (i + 1) * 90 / CIRCLE_COUNT;
        if (theta != 0)
        {
            Vector3f &pos = vertices[i*VERTEX_PER_CIRCLE_COUNT];
            mThetaLabels.push_back(make_pair(to_string(theta), pos + Vector3f{0.02f, 0.02f, -0.02f}));
        }
    }

    for (unsigned int i = 0; i < LINE_COUNT; ++i)
    {
        unsigned int index = CIRCLE_COUNT * VERTEX_PER_CIRCLE_COUNT + i * VERTEX_PER_LINE_COUNT;
        double angle = M_PI * i / LINE_COUNT;
        vertices[index] = { (float)cos(angle), 0, (float)sin(angle) };
        vertices[index + 1] = -vertices[index];

        mPhiLabels.push_back(make_pair(to_string(180 * i / LINE_COUNT), vertices[index] + vertices[index].normalized() * 0.04f));
        mPhiLabels.push_back(make_pair(to_string(180 * i / LINE_COUNT + 180), vertices[index+1] + vertices[index + 1].normalized() * 0.04f));
    }

    mShader.bind();
    mShader.uploadAttrib("in_pos", vertices.size(), 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)vertices.data());
}

RadialGrid::~RadialGrid()
{
    mShader.free();
}

void RadialGrid::drawGL(
    const Matrix4f& model,
    const Matrix4f& view,
    const Matrix4f& proj)
{
    if (mVisible)
    {
        Matrix4f mvp = proj * view * model;
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        mShader.bind();
        mShader.setUniform("modelViewProj", mvp);

        for (size_t j = 0; j < 2; j++)
        {
            glDepthFunc(j % 2 == 0 ? GL_LESS : GL_GREATER);
            mShader.setUniform("in_color", j % 2 == 0 ? mColor : mColor * 0.6);
            for (unsigned int i = 0; i < CIRCLE_COUNT; ++i)
            {
                mShader.drawArray(GL_LINE_LOOP, i*VERTEX_PER_CIRCLE_COUNT, VERTEX_PER_CIRCLE_COUNT);
            }
            mShader.drawArray(GL_LINES, CIRCLE_COUNT*VERTEX_PER_CIRCLE_COUNT, LINE_COUNT*VERTEX_PER_LINE_COUNT);
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
    if (mVisible && mShowDegrees)
    {
        Matrix4f mvp = proj * view * model;
        nvgFontSize(ctx, 15.0f);
        nvgFontFace(ctx, "sans");
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(ctx, mColor);
        for (const auto& phiLabel : mPhiLabels)
        {
            Vector4f projectedPoint = projectOnScreen(phiLabel.second, canvasSize, mvp);
            nvgText(ctx, projectedPoint[0], projectedPoint[1], phiLabel.first.c_str(), nullptr);
        }
        for (const auto& thetaLabel : mThetaLabels)
        {
            Vector4f projectedPoint = projectOnScreen(thetaLabel.second, canvasSize, mvp);
            nvgText(ctx, projectedPoint[0], projectedPoint[1], thetaLabel.first.c_str(), nullptr);
        }
    }
}

TEKARI_NAMESPACE_END
