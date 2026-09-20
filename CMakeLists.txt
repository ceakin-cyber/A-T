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

FetchContent_MakeAvailable(glfw glad)

add_executable(a-t src/main.cpp)
target_link_libraries(a-t PRIVATE glfw glad)
# glad provides the GL headers; keep GLFW from including its own.
target_compile_definitions(a-t PRIVATE GLFW_INCLUDE_NONE)
