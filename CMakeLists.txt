cmake_minimum_required (VERSION 2.8.3)
project(tekari)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/glfw")
  message(FATAL_ERROR "Tekari's dependency repositories (NanoGUI, GLFW, etc.) are missing! "
    "You probably did not clone the project with --recursive. It is possible to recover "
    "by calling \"git submodule update --init --recursive\"")
endif()

# Enable folders for projects in Visual Studio
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Sanitize build environment for static build with C++11
if (MSVC)
  add_definitions (/D "_CRT_SECURE_NO_WARNINGS")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")

    # Disable Eigen vectorization for Windows 32 bit builds (issues with unaligned access segfaults)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DEIGEN_DONT_ALIGN")
  endif()

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

# Enable C++14 mode on GCC / Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)

macro(CHECK_CXX_COMPILER_AND_LINKER_FLAGS _RESULT _CXX_FLAGS _LINKER_FLAGS)
  set(CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
  set(CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_source_runs("#include <iostream>\nint main(int argc, char **argv) { std::cout << \"test\"; return 0; }" ${_RESULT})
  set(CMAKE_REQUIRED_FLAGS "")
  set(CMAKE_REQUIRED_LIBRARIES "")
endmacro()

# Build NanoGUI
set(NANOGUI_BUILD_EXAMPLE OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_SHARED  OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui ext_build/nanogui)
set_property(TARGET nanogui PROPERTY FOLDER "dependencies")
# set_property(TARGET nanogui glfw glfw_objects PROPERTY FOLDER "dependencies")

# Prefer libc++ in conjunction with Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  if (CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    message(STATUS "NanoGUI: using libc++.")
  else()
  CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
  if (HAS_LIBCPP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
      message(STATUS "NanoGUI: using libc++.")
  else()
    message(STATUS "NanoGUI: NOT using libc++.")
  endif()
  endif()
endif()

# Set compilation flags for Enoki
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/ext/enoki)
enoki_set_compile_flags()
enoki_set_native_flags()

# if (CMAKE_CXX_COMPILER_ID MATCHES "Emscripten")
  # add_library(tbb_static
  #     "ext/tbb_dummy/include/tbb/parallel_for.h"
  #   )
  # set_target_properties(tbb_static PROPERTIES LINKER_LANGUAGE CXX)
  # message(STATUS "TBB: using dummy implementation.")
# else()
  # Build TBB
  set(TBB_BUILD_STATIC ON CACHE BOOL " " FORCE)
  set(TBB_BUILD_SHARED OFF CACHE BOOL " " FORCE)
  set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL " " FORCE)
  set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL " " FORCE)
  set(TBB_BUILD_TESTS OFF CACHE BOOL " " FORCE)

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/tbb ext_build/tbb)
  set_property(TARGET tbb_static tbb_def_files PROPERTY FOLDER "dependencies")
  message(STATUS "TBB: NOT using dummy implementation.")
# endif()


# Build Triangle
# Preprocessor constant to make triangle use floats instead of doubles
add_definitions(-DSINGLE)
add_definitions(-DTRILIBRARY)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/triangle ext_build/triangle)
set_property(TARGET triangle PROPERTY FOLDER "dependencies")

# Compile tekari with various compiler warnings turned on
if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-anonymous-struct -Wno-c99-extensions -Wno-nested-anon-types")
  endif()
endif()

# Header file directories of dependency libraries
include_directories(
  # NanoGUI
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include
  # NanoGUI dependencies (Eigen, GLFW, NanoVG, GLAD?)
  ${NANOGUI_EXTRA_INCS}
  # Triangle
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/triangle

  # Tekari's include directory
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# Conditionaly add TBB include directory
if (NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Emscripten")
  include_directories(
    # Intel Thread Building Blocks
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tbb/include
  )
endif()

add_executable(tekari
  "src/main.cpp"
  "include/tekari/BSDFApplication.h"            "src/BSDFApplication.cpp"
  "include/tekari/BSDFCanvas.h"                 "src/BSDFCanvas.cpp"
  "include/tekari/DataSample.h"                 "src/DataSample.cpp"
  "include/tekari/RadialGrid.h"                 "src/RadialGrid.cpp"
  "include/tekari/DataSampleButton.h"           "src/DataSampleButton.cpp"
  "include/tekari/Metadata.h"                   "src/Metadata.cpp"
  "include/tekari/ColorMap.h"                   "src/ColorMap.cpp" 
  "include/tekari/ColorMapSelectionWindow.h"    "src/ColorMapSelectionWindow.cpp" 
  "include/tekari/ColorMapButton.h"             "src/ColorMapButton.cpp"
  "include/tekari/MetadataWindow.h"             "src/MetadataWindow.cpp"
  "include/tekari/HelpWindow.h"                 "src/HelpWindow.cpp"
  "include/tekari/Axis.h"                       "src/Axis.cpp"
  "include/tekari/selections.h"                 "src/selections.cpp"
  "include/tekari/raw_data_processing.h"        "src/raw_data_processing.cpp"
  "include/tekari/points_stats.h"               "src/points_stats.cpp"
  "include/tekari/data_io.h"                    "src/data_io.cpp"
  "include/tekari/Arrow.h"
  "include/tekari/LightTheme.h"
  "include/tekari/ThreadPool.h"
  "include/tekari/SharedQueue.h"
  "include/tekari/common.h"
  "include/tekari/stop_watch.h"
  tekari.rc
  )

add_executable(tests
  "src/tests.cpp"
  )

add_definitions(${NANOGUI_EXTRA_DEFS})
target_link_libraries(tekari tbb_static nanogui triangle ${NANOGUI_EXTRA_LIBS})
target_link_libraries(tests tbb_static nanogui triangle ${NANOGUI_EXTRA_LIBS})

set_target_properties(tekari PROPERTIES OUTPUT_NAME "tekari")
set_target_properties(tests PROPERTIES OUTPUT_NAME "tests")
