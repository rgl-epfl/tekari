cmake_minimum_required (VERSION 3.1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

if (CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$")
  set(CMAKE_CXX_COMPILER_ID "Emscripten")
  set(CMAKE_STATIC_LIBRARY_SUFFIX ".bc")
  set(CMAKE_EXECUTABLE_SUFFIX ".bc")
  set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_CXX_COMPILER> -o <TARGET> <LINK_FLAGS> <OBJECTS>")
  if (U_CMAKE_BUILD_TYPE MATCHES REL)
    add_compile_options(-O3 -DNDEBUG)
  endif()
endif()

# Sanitize build environment for static build with C++11
if (MSVC)
  add_definitions (/D "_CRT_SECURE_NO_WARNINGS")

  # Parallel build on MSVC (all targets)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# Enable C++14 mode on GCC / Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|Emscripten")
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
set(NANOGUI_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_SHARED   OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON   OFF CACHE BOOL " " FORCE)
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
if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Emscripten")
  enoki_set_compile_flags()
  enoki_set_native_flags()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Emscripten")
  message(STATUS "TBB: using dummy implementation.")
else()
  # Build TBB
  set(TBB_BUILD_STATIC ON CACHE BOOL " " FORCE)
  set(TBB_BUILD_SHARED OFF CACHE BOOL " " FORCE)
  set(TBB_BUILD_TBBMALLOC OFF CACHE BOOL " " FORCE)
  set(TBB_BUILD_TBBMALLOC_PROXY OFF CACHE BOOL " " FORCE)
  set(TBB_BUILD_TESTS OFF CACHE BOOL " " FORCE)

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/tbb ext_build/tbb)
  set_property(TARGET tbb_static tbb_def_files PROPERTY FOLDER "dependencies")
endif()


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

# Run simple cmake converter (from Nanogui) to put resource files into the data segment
file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/*"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/webgl_shaders/*"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/color_maps/*"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/*")

# Concatenate resource files into a comma separated string
string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

# Create command line for running bin2c cmake script
set(bin2c_cmdline
  -DOUTPUT_C=tekari_resources.cpp
  -DOUTPUT_H=tekari_resources.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/resources/bin2c.cmake")

# Run bin2c on resource files
add_custom_command(
  OUTPUT tekari_resources.cpp tekari_resources.h
  COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
  DEPENDS ${resources}
  COMMENT "Running bin2c"
  PRE_BUILD VERBATIM)

# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(${NANOGUI_EXTRA_DEFS})

# Header file directories of dependency libraries
include_directories(
  # NanoGUI
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/include # NanoGUI dependencies (Eigen, GLFW, NanoVG, GLAD?)
  ${NANOGUI_EXTRA_INCS}
  # Triangle
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/triangle

  # Tekari's include directory
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# Conditionaly add TBB include directory
# if (${CMAKE_CXX_COMPILER_ID} MATCHES "Emscripten")
  include_directories(
    # Intel Thread Building Blocks
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tbb_dummy/include
  )
# else()
#   include_directories(
#     # Intel Thread Building Blocks
#     ${CMAKE_CURRENT_SOURCE_DIR}/ext/tbb/include
#   )
# endif()

add_executable(tekari
  src/main.cpp
  include/tekari/bsdf_application.h             src/bsdf_application.cpp
  include/tekari/bsdf_canvas.h                  src/bsdf_canvas.cpp
  include/tekari/data_sample.h                  src/data_sample.cpp
  include/tekari/radial_grid.h                  src/radial_grid.cpp
  include/tekari/data_sample_button.h           src/data_sample_button.cpp
  include/tekari/metadata.h                     src/metadata.cpp
  include/tekari/color_map.h                    src/color_map.cpp 
  include/tekari/color_map_selection_window.h   src/color_map_selection_window.cpp 
  include/tekari/color_map_button.h             src/color_map_button.cpp
  include/tekari/metadata_window.h              src/metadata_window.cpp
  include/tekari/help_window.h                  src/help_window.cpp
  include/tekari/axis.h                         src/axis.cpp
  include/tekari/selections.h                   src/selections.cpp
  include/tekari/raw_data_processing.h          src/raw_data_processing.cpp
  include/tekari/points_stats.h                 src/points_stats.cpp
  include/tekari/data_io.h                      src/data_io.cpp
  include/tekari/arrow.h                        src/arrow.cpp
  include/tekari/slider_2d.h                    src/slider_2d.cpp
  include/tekari/powitacq.h                     include/tekari/powitacq.inl
  include/tekari/bsdf_data_sample.h             src/bsdf_data_sample.cpp
  include/tekari/standard_data_sample.h
  include/tekari/matrix_xx.h
  include/tekari/raw_measurement.h
  include/tekari/light_theme.h
  include/tekari/thread_pool.h
  include/tekari/shared_queue.h
  include/tekari/common.h
  include/tekari/stop_watch.h
  ${CMAKE_CURRENT_BINARY_DIR}/tekari_resources.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/tekari_resources.h
  tekari.rc
)

add_executable(tests
  "src/tests.cpp"
)

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Emscripten")
  set(NANOGUI_EXTRA_LIBS ${NANOGUI_EXTRA_LIBS} tbb_static)
endif()

target_link_libraries(tekari nanogui triangle ${NANOGUI_EXTRA_LIBS})

set_target_properties(tekari PROPERTIES OUTPUT_NAME "tekari")
set_target_properties(tests PROPERTIES OUTPUT_NAME "tests")
