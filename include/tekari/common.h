#pragma once

#include <nanogui/glutil.h>

#if defined(_WIN32)
#	define NOMINMAX  // Remove min/max macros when building on windows
#	include <Windows.h>
#	undef NOMINMAX
#   undef near      // also cleanup some macros conflicting with variable names
#   undef far
#	pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#	pragma warning(disable : 4244) // warning C4244: conversion from X to Y, possible loss of data
#	pragma warning(disable : 4251) // warning C4251: class X needs to have dll-interface to be used by clients of class Y
#	pragma warning(disable : 4714) // warning C4714: function X marked as __forceinline not inlined
#endif

// define M_PI locally since it's not necessarily defined on some platforms
#undef M_PI // make sure we don't define M_PI twice
#define M_PI 3.141592653589793238463

// Define command key for windows/mac/linux
#if defined(__APPLE__) || defined(DOXYGEN_DOCUMENTATION_BUILD)
	/// If on OSX, maps to ``GLFW_MOD_SUPER``.  Otherwise, maps to ``GLFW_MOD_CONTROL``.
	#define SYSTEM_COMMAND_MOD GLFW_MOD_SUPER
#else
	#define SYSTEM_COMMAND_MOD GLFW_MOD_CONTROL
#endif

#define TEKARI_NAMESPACE_BEGIN namespace tekari {
#define TEKARI_NAMESPACE_END }

TEKARI_NAMESPACE_BEGIN

inline nanogui::Vector4f projectOnScreen(const nanogui::Vector3f &point,
    const nanogui::Vector2i &canvasSize,
    const nanogui::Matrix4f &mvp)
{
    nanogui::Vector4f homogeneousPoint;
    homogeneousPoint << point, 1.0f;
    nanogui::Vector4f projectedPoint{ mvp * homogeneousPoint };

    projectedPoint /= projectedPoint[3];
    projectedPoint[0] = (projectedPoint[0] + 1.0f) * 0.5f * canvasSize.x();
    projectedPoint[1] = canvasSize.y() - (projectedPoint[1] + 1.0f) * 0.5f * canvasSize.y();
    return projectedPoint;
}

TEKARI_NAMESPACE_END