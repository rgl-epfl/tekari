#include "tekari/Axis.h"

#pragma once

#include <nanogui/glutil.h>
#include <string>
#include <iostream>

TEKARI_NAMESPACE_BEGIN

using namespace nanogui;

Axis::Axis(Vector3f origin)
    : m_Origin(origin)
{}
Axis::~Axis()
{
    m_Shader.free();
}

void Axis::loadShader()
{
    const std::string shader_path = "../resources/shaders/";
    m_Shader.initFromFiles("axis", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");
    m_Shader.bind();
    m_Shader.setUniform("length", 0.15f);
    const float pos[] = { 0.0f, 0.0f, 0.0f };
    m_Shader.uploadAttrib("pos", 1, 3, sizeof(Vector3f), GL_FLOAT, GL_FALSE, (const void*)pos);
    m_Shader.setUniform("origin", m_Origin);
}

void Axis::drawGL(const Matrix4f& mvp)
{
    glEnable(GL_DEPTH_TEST);
    m_Shader.bind();
    m_Shader.setUniform("origin", m_Origin);
    m_Shader.setUniform("modelViewProj", mvp);

    m_Shader.setUniform("direction", Vector3f{ 1, 0, 0 });
    m_Shader.setUniform("color", Vector3f{ 1, 0, 0 });
    m_Shader.drawArray(GL_POINTS, 0, 1);

    m_Shader.setUniform("direction", Vector3f{ 0, 1, 0 });
    m_Shader.setUniform("color", Vector3f{ 0, 1, 0 });
    m_Shader.drawArray(GL_POINTS, 0, 1);

    m_Shader.setUniform("direction", Vector3f{ 0, 0, 1 });
    m_Shader.setUniform("color", Vector3f{ 0, 0, 1 });
    m_Shader.drawArray(GL_POINTS, 0, 1);
    glDisable(GL_DEPTH_TEST);
}

void Axis::setOrigin(const Vector3f& newOrigin)
{
    m_Origin = newOrigin;
}

TEKARI_NAMESPACE_END