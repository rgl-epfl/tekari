#include "Axis.h"

#pragma once

#include <nanogui/glutil.h>
#include <string>

Axis::Axis(nanogui::Vector3f origin)
    : m_Origin(origin)
{
    const std::string shader_path = "../resources/shaders/";
    m_Shader.initFromFiles("axis", shader_path + "axis.vert", shader_path + "axis.frag", shader_path + "axis.geom");
    m_Shader.bind();
    m_Shader.uploadAttrib("pos", 1, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)&m_Origin[0]);
}
Axis::~Axis()
{
    m_Shader.free();
}

void Axis::drawGL(
    const nanogui::Matrix4f& model,
    const nanogui::Matrix4f& view,
    const nanogui::Matrix4f& proj)
{
    glEnable(GL_DEPTH_TEST);
    m_Shader.bind();
    m_Shader.setUniform("modelViewProj", nanogui::Matrix4f(proj*view*model));
    m_Shader.drawArray(GL_POINTS, 0, 1);
    glDisable(GL_DEPTH_TEST);
}

void Axis::setOrigin(nanogui::Vector3f newOrigin)
{
    m_Origin = newOrigin;
    m_Shader.bind();
    m_Shader.uploadAttrib("pos", 1, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)&m_Origin[0]);
}
