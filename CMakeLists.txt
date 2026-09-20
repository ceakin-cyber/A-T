cmake_minimum_required(VERSION 3.20)

project(A-T VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

# Pre-generated glad loader (OpenGL 4.1 core), so the build needs no Python.
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/libigl/libigl-glad.git
  GIT_TAG        651a425101365aa6e8504988ef9bb363d066c5ee
)

# Dear ImGui (docking branch) has no CMake build, so we compile its sources ourselves.
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.9b-docking
  GIT_SHALLOW    TRUE
)

# HTTP client. Uses the system libcurl (libcurl4-openssl-dev) instead of building curl.
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG        1.14.2
  GIT_SHALLOW    TRUE
)
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS     OFF CACHE BOOL "" FORCE)

# Unit tests.
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
  GIT_SHALLOW    TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw glad imgui cpr googletest)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

# Astronomy and orbit code. Must not depend on the UI or network code.
add_library(core STATIC src/core/frames.cpp src/core/kepler.cpp src/core/sgp4.cpp src/core/time.cpp src/core/tle.cpp)
target_include_directories(core PUBLIC src)

# Network access and the on-disk TLE cache.
add_library(net STATIC src/net/tle_fetch.cpp src/net/tle_cache.cpp src/net/tle_source.cpp)
target_include_directories(net PUBLIC src)
target_link_libraries(net PRIVATE cpr::cpr)

add_executable(a-t src/main.cpp src/ui/header.cpp src/ui/style.cpp)
target_include_directories(a-t PRIVATE src)
target_link_libraries(a-t PRIVATE core net glfw glad imgui)
# glad provides the GL headers; keep GLFW from including its own.
target_compile_definitions(a-t PRIVATE GLFW_INCLUDE_NONE)

# Copy assets next to the executable so it finds them regardless of the working directory.
add_custom_command(TARGET a-t POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:a-t>/assets
)

enable_testing()
add_executable(tests tests/frames_test.cpp tests/kepler_test.cpp tests/sgp4_init_test.cpp tests/sgp4_propagate_test.cpp tests/sgp4_secular_test.cpp tests/time_test.cpp tests/tle_test.cpp tests/tle_cache_test.cpp tests/tle_source_test.cpp)
target_link_libraries(tests PRIVATE core net GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)
