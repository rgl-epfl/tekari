#include "tekari/Axis.h"

#pragma once

#include <nanogui/glutil.h>
#include <string>
#include <iostream>

TEKARI_NAMESPACE_BEGIN

using namespace nanogui;

Axis::Axis(Vector3f origin)
    : mOrigin(origin)
{}
Axis::~Axis()
{
    mShader.free();
}

void Axis::loadShader()
{
    const std::string shader_path = "../resources/shaders/";
    mShader.initFromFiles("axis", shader_path + "arrow.vert", shader_path + "arrow.frag", shader_path + "arrow.geom");
    mShader.bind();
    mShader.setUniform("length", 0.15f);
    mShader.uploadAttrib("pos", Vector3f{ 0, 0, 0 });
    mShader.setUniform("origin", mOrigin);
}

void Axis::drawGL(const Matrix4f& mvp)
{
    glEnable(GL_DEPTH_TEST);
    mShader.bind();
    mShader.setUniform("origin", mOrigin);
    mShader.setUniform("modelViewProj", mvp);

    mShader.setUniform("direction", Vector3f{ 1, 0, 0 });
    mShader.setUniform("color", Vector3f{ 1, 0, 0 });
    mShader.drawArray(GL_POINTS, 0, 1);

    mShader.setUniform("direction", Vector3f{ 0, 1, 0 });
    mShader.setUniform("color", Vector3f{ 0, 1, 0 });
    mShader.drawArray(GL_POINTS, 0, 1);

    mShader.setUniform("direction", Vector3f{ 0, 0, 1 });
    mShader.setUniform("color", Vector3f{ 0, 0, 1 });
    mShader.drawArray(GL_POINTS, 0, 1);
    glDisable(GL_DEPTH_TEST);
}

void Axis::setOrigin(const Vector3f& newOrigin)
{
    mOrigin = newOrigin;
}

TEKARI_NAMESPACE_END