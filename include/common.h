#pragma once

#if defined(_WIN32)
#	define NOMINMAX  // Remove min/max macros when building on windows
#	include <Windows.h>
#	undef NOMINMAX
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
