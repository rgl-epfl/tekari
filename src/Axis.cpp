#include "Axis.h"

#pragma once

#include <nanogui/glutil.h>
#include <string>

Axis::Axis(nanogui::Vector3f origin)
    : m_Origin(origin)
{}
Axis::~Axis()
{
    m_Shader.free();
}

void Axis::loadShader()
{
    float data[] = { 0.0f, 0.0f, 0.0f };
    const std::string shader_path = "../resources/shaders/";
    m_Shader.initFromFiles("axis", shader_path + "axis.vert", shader_path + "axis.frag", shader_path + "axis.geom");
    m_Shader.bind();
    m_Shader.uploadAttrib("pos", 1, 3, sizeof(float) * 3, GL_FLOAT, GL_FALSE, (const void*)data);
}

void Axis::drawGL(const nanogui::Matrix4f& mvp)
{
    glEnable(GL_DEPTH_TEST);
    m_Shader.bind();
    m_Shader.setUniform("modelViewProj", mvp);
    m_Shader.setUniform("origin", m_Origin);
    m_Shader.drawArray(GL_POINTS, 0, 1);
    glDisable(GL_DEPTH_TEST);
}

void Axis::setOrigin(const nanogui::Vector3f& newOrigin)
{
    m_Origin = newOrigin;
}
