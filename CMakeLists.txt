cmake_minimum_required (VERSION 2.8.3)
project(BSDFVisualizer)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

# Sanitize build environment for static build with C++11
if (MSVC)
  add_definitions (/D "_CRT_SECURE_NO_WARNINGS")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  # Static build
  set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  foreach(CompilerFlag ${CompilerFlags})
    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  endforeach()
endif()

# Build qhull
set(qhull_STATIC ON CACHE BOOL " " FORCE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext/qhull/src)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/qhull/ ext_build/qhull)
#set_property(TARGET qhullstatic PROPERTY FOLDER "dependencies")

# Enable C++14 mode on GCC / Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

# Header file directories of dependency libraries
include_directories(
  # qhull
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/qhull/src

  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(BSDFVisualizer
  src/main.cpp
  )

target_link_libraries(BSDFVisualizer qhullstatic qhullcpp)

set_target_properties(BSDFVisualizer PROPERTIES OUTPUT_NAME "BSDF Visualizer")
