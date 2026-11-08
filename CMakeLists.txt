cmake_minimum_required(VERSION 3.27)
project(freya)

### Project Options ###
############################################################
option(FREYA_BUILD_SHARED  "Build Freya as a shared library" OFF)
option(FREYA_BUILD_TESTBED "Build the testbeds with Freya"   ON)
option(FREYA_DISTRIBUTE    "Enable the distribution build"   OFF)

if(FREYA_BUILD_SHARED)
  set(FREYA_BUILD_TYPE SHARED)
  set(BUILD_SHARED_LIBS ON)
else()
  set(FREYA_BUILD_TYPE STATIC)
  set(BUILD_SHARED_LIBS OFF)
endif()

if(FREYA_DISTRIBUTE)
  add_definitions("-DFREYA_BUILD_DISTRIBUTION")
endif()
############################################################

### FetchContent ###
############################################################
include(FetchContent)

if(WIN32)
  if(USE_STATIC_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING FORCE)
  endif()
endif()

# GLM

if(NOT DEFINED EMSCRIPTEN)
  set(GLM_ENABLE_SIMD_AVX ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
  glm 
  URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
)

# LUA (fork, because it's CMake-based)

FetchContent_Declare(
  lua 
  GIT_REPOSITORY https://github.com/walterschell/Lua.git
)

# ImGui

FetchContent_Declare(
  imgui 
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8-docking.zip
)

# Tracy

FetchContent_Declare(
  tracy 
  URL https://github.com/wolfpld/tracy/archive/refs/tags/v0.13.1.zip
)


# Assimp 

set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_SAMPLES                  OFF)
set(ASSIMP_BUILD_TESTS                    OFF)
set(ASSIMP_BUILD_ASSIMP_VIEW              OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF)
set(ASSIMP_NO_EXPORT                      OFF)
set(ASSIMP_INSTALL                        OFF)

set(ASSIMP_BUILD_FBX_IMPORTER  ON)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)

FetchContent_Declare(
  assimp 
  URL https://github.com/assimp/assimp/archive/refs/tags/v6.0.5.zip
)

# Fetch everything
FetchContent_MakeAvailable(glm imgui lua tracy assimp)

if(NOT DEFINED EMSCRIPTEN) # These libraries are not required when building for the web
  # GLFW

  FetchContent_Declare(
    glfw-3.4 
    URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
  )

  # OpenAL-Soft

  set(ALSOFT_EXAMPLES        OFF CACHE BOOL "" FORCE)
  set(ALSOFT_DLOPEN          OFF CACHE BOOL "" FORCE)
  set(ALSOFT_UTILS           OFF CACHE BOOL "" FORCE)
  set(ALSOFT_INSTAL_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(ALSOFT_INSTAL_UTILS    OFF CACHE BOOL "" FORCE)

  set(LIBTYPE STATIC)

  FetchContent_Declare(
    OpenAL 
    URL https://github.com/kcat/openal-soft/archive/refs/tags/1.25.0.zip
  )

  FetchContent_MakeAvailable(glfw-3.4 OpenAL)
endif()
############################################################

### Project Variables ###
############################################################
set(FREYA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/freya/src)
set(FREYA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/freya/include)
set(FREYA_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(FREYA_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/freya)

set(FREYA_INCLUDES 
  ${FREYA_INCLUDE_DIR}
  ${FREYA_LIBS_DIR}

  ${glfw-3.4_SOURCE_DIR}/include
  ${glm_SOURCE_DIR}
  ${OpenAL_SOURCE_DIR}/include
  ${lua_SOURCE_DIR}/lua-5.4.7/include

  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${imgui_SOURCE_DIR}/misc/cpp

  ${tracy_SOURCE_DIR}/public
  ${assimp_SOURCE_DIR}/include
)

set(FREYA_COMMON_LIBRARIES 
  lua_static
  assimp
)

set(FREYA_LIBRARIES ${FREYA_COMMON_LIBRARIES})

if(NOT DEFINED EMSCRIPTEN)
  set(FREYA_LIBRARIES "${FREYA_COMMON_LIBRARIES};glfw;OpenAL")
else()
  set(FREYA_LIBRARIES "${FREYA_COMMON_LIBRARIES};openal;glfw")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# This script includes a bunch of platform-specific variables 
# and definitions that will be helpful later on. 
include(cmake/utils.cmake)
############################################################

### Sub Directories ###
############################################################
add_subdirectory(freya)

if(FREYA_BUILD_TESTBED) 
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/testbed)
endif()
############################################################

### Library Install ###
############################################################
install(TARGETS ${PROJECT_NAME} 
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib 
        ARCHIVE DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/freya/include DESTINATION include)
############################################################
