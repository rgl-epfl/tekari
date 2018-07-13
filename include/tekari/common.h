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

#include <algorithm>
#include <cctype>
#include <nanogui/glutil.h>
#include <enoki/array.h>
#include <enoki/matrix.h>

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

using Vector2f = nanogui::Vector2f;
using Vector3f = nanogui::Vector3f;
using Vector4f = nanogui::Vector4f;
using Vector2i = nanogui::Vector2i;
using Vector3u = enoki::Array<uint32_t, 3>;

using Matrix3f = enoki::Matrix<float, 3>;
using Matrix4f = nanogui::Matrix4f;

using Matrix2Xf = std::vector<Vector2f>;
using Matrix3Xf = std::vector<Vector3f>;
using MatrixXXf = std::vector<std::vector<float>>;
using Matrix3Xu = std::vector<Vector3u>;
using VectorXu  = std::vector<uint32_t>;
using VectorXi8 = std::vector<int8_t>;
using VectorXf  = std::vector<float>;

using Quaternion4f = nanogui::Quaternion4f;

using Index = size_t;

inline Vector4f project_on_screen(const Vector3f& point,
    const Vector2i& canvas_size,
    const Matrix4f& mvp)
{
    Vector4f projected_point{ mvp* concat(point, 1.0f) };

    projected_point /= projected_point[3];
    projected_point[0] = (projected_point[0] + 1.0f)* 0.5f* canvas_size.x();
    projected_point[1] = canvas_size.y() - (projected_point[1] + 1.0f)* 0.5f* canvas_size.y();
    return projected_point;
}

inline Vector3f get3DPoint(const Matrix2Xf& V2D, const VectorXf& H, unsigned int index)
{
    return Vector3f{ V2D[index][0], H[index], V2D[index][1] };
}

inline Matrix3Xf get3DPoints(const Matrix2Xf& V2D, const std::vector<VectorXf>& H, unsigned int index)
{
    Matrix3Xf result;
    result.resize(H.size()) ;
    for (size_t i = 0; i < H.size(); i++)
    {
        result[i] = get3DPoint(V2D, H[i], index);
    }
    return result;
}

inline Vector2f transform_raw_point(const Vector2f& raw2DPoint)
{

    return Vector2f{   (float)(raw2DPoint[0]* cos(raw2DPoint[1]* M_PI / 180.0f) / 90.0f),
                       (float)(raw2DPoint[0]* sin(raw2DPoint[1]* M_PI / 180.0f) / 90.0f) };
}


// ================= String Utils ================

// trim from start (in place)
inline void ltrim(std::string &s, std::function<int(int)> pred = [](int c) { return std::isspace(c); }) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [pred](int ch) {
        return !pred(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string &s, std::function<int(int)> pred = [](int c) { return std::isspace(c); }) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [pred](int ch) {
        return !pred(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s, std::function<int(int)> pred = [](int c) { return std::isspace(c); }) {
    ltrim(s, pred);
    rtrim(s, pred);
}

// trim from start (copying)
inline std::string ltrim_copy(std::string s, std::function<int(int)> pred = [](int c) { return std::isspace(c); }) {
    ltrim(s, pred);
    return s;
}

// trim from end (copying)
inline std::string rtrim_copy(std::string s, std::function<int(int)> pred = [](int c) { return std::isspace(c); }) {
    rtrim(s, pred);
    return s;
}

// trim from both ends (copying)
inline std::string trim_copy(std::string s, std::function<int(int)> pred = [](int c) { return std::isspace(c); }) {
    trim(s, pred);
    return s;
}

TEKARI_NAMESPACE_END