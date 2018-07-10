#pragma once

#if defined(_WIN32)
#    define NOMINMAX  // Remove min/max macros when building on windows
#    include <Windows.h>
#    undef NOMINMAX
#   undef near      // also cleanup some macros conflicting with variable names
#   undef far
#    pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#    pragma warning(disable : 4244) // warning C4244: conversion from X to Y, possible loss of data
#    pragma warning(disable : 4251) // warning C4251: class X needs to have dll-interface to be used by clients of class Y
#    pragma warning(disable : 4714) // warning C4714: function X marked as __forceinline not inlined
#endif

#include <nanogui/glutil.h>

// (re)define M_PI locally since it's not necessarily defined on some platforms
#undef M_PI
#define M_PI 3.141592653589793238463

#define ENABLE_PROFILING

// Define command key for windows/mac/linux
#if defined(__APPLE__) || defined(DOXYGEN_DOCUMENTATION_BUILD)
    /// If on OSX, maps to ``GLFW_MOD_SUPER``.  Otherwise, maps to ``GLFW_MOD_CONTROL``.
    #define SYSTEM_COMMAND_MOD GLFW_MOD_SUPER
#else
    #define SYSTEM_COMMAND_MOD GLFW_MOD_CONTROL
#endif

#define TEKARI_NAMESPACE_BEGIN namespace tekari {
#define TEKARI_NAMESPACE_END }

#define GRAIN_SIZE 1024u

TEKARI_NAMESPACE_BEGIN

using MatrixXf  = nanogui::MatrixXf;
using MatrixXu  = nanogui::MatrixXu;
using VectorXu  = Eigen::Matrix<unsigned int, 1, Eigen::Dynamic>;
using VectorXu8 = Eigen::Matrix<uint8_t, 1, Eigen::Dynamic>;
using VectorXf  = Eigen::Matrix<float, 1, Eigen::Dynamic>;

using nanogui::Matrix4f;
using nanogui::Vector4f;
using nanogui::Vector3f;
using nanogui::Vector2f;
using nanogui::Vector2i;

inline Vector4f project_on_screen(const Vector3f &point,
    const Vector2i &canvas_size,
    const Matrix4f &mvp)
{
    Vector4f homogeneous_point;
    homogeneous_point << point, 1.0f;
    Vector4f projected_point{ mvp * homogeneous_point };

    projected_point /= projected_point[3];
    projected_point[0] = (projected_point[0] + 1.0f) * 0.5f * canvas_size.x();
    projected_point[1] = canvas_size.y() - (projected_point[1] + 1.0f) * 0.5f * canvas_size.y();
    return projected_point;
}

inline MatrixXf get3DPoint(const MatrixXf &V2D, const VectorXf &H, unsigned int index)
{
    return Vector3f{ V2D(0, index), H(index), V2D(1, index) };
}

inline MatrixXf get3DPoints(const MatrixXf &V2D, const std::vector<VectorXf> &H, unsigned int index)
{
    MatrixXf result;
    result.resize(3, H.size());
    for (size_t i = 0; i < H.size(); i++)
    {
        result.col(i) = get3DPoint(V2D, H[i], index);
    }
    return result;
}

inline Vector2f transform_raw_point(const Vector2f& raw2DPoint)
{
    return Vector2f{   (float)(raw2DPoint[0] * cos(raw2DPoint[1] * M_PI / 180.0f) / 90.0f),
                       (float)(raw2DPoint[0] * sin(raw2DPoint[1] * M_PI / 180.0f) / 90.0f) };
}

TEKARI_NAMESPACE_END