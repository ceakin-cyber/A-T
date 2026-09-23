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
add_library(core STATIC src/core/celestial.cpp src/core/constellation.cpp src/core/frames.cpp src/core/geodetic.cpp src/core/kepler.cpp src/core/lunar.cpp src/core/passes.cpp src/core/sgp4.cpp src/core/star_catalog.cpp src/core/time.cpp src/core/tle.cpp src/core/topocentric.cpp)
target_include_directories(core PUBLIC src)

# Network access and the on-disk TLE cache.
add_library(net STATIC src/net/tle_fetch.cpp src/net/tle_cache.cpp src/net/tle_source.cpp)
target_include_directories(net PUBLIC src)
target_link_libraries(net PRIVATE cpr::cpr)

# Glue between the network layer and the astronomy code.
add_library(app STATIC src/app/config.cpp src/app/event_log.cpp src/app/format.cpp src/app/ground_track.cpp src/app/operating_rules.cpp src/app/pass_planner.cpp src/app/satellite_position.cpp src/app/satellite_roster.cpp src/app/star_map_time.cpp src/app/system_status.cpp src/app/tle_age.cpp src/app/tracked_satellite.cpp src/app/transmission_events.cpp src/app/typing_effect.cpp)
target_include_directories(app PUBLIC src)
target_link_libraries(app PUBLIC core net)

add_executable(a-t src/main.cpp src/ui/bloom_chain.cpp src/ui/crt_tuning_panel.cpp src/ui/dockspace.cpp src/ui/event_log_panel.cpp src/ui/framebuffer.cpp src/ui/fullscreen_triangle.cpp src/ui/ground_track_panel.cpp src/ui/header.cpp src/ui/incoming_transmission_panel.cpp src/ui/log_panel.cpp src/ui/lunar_alignment_panel.cpp src/ui/pass_panel.cpp src/ui/rows.cpp src/ui/screen_pass.cpp src/ui/shader.cpp src/ui/star_map_panel.cpp src/ui/style.cpp src/ui/system_status_panel.cpp src/ui/tracker_panel.cpp src/ui/watchlist_panel.cpp)
target_include_directories(a-t PRIVATE src)
target_link_libraries(a-t PRIVATE app core net glfw glad imgui)
# glad provides the GL headers; keep GLFW from including its own.
target_compile_definitions(a-t PRIVATE GLFW_INCLUDE_NONE)

# Copy assets next to the executable so it finds them regardless of the working directory. This
# runs on every build, not only when the executable is relinked, so edits to a shader or font
# show up without touching any C++ file.
add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:a-t>/assets
)
add_dependencies(a-t copy_assets)

# Installing (and packaging, below) puts the executable and assets/ side by side in the same
# directory, matching the layout the app expects at runtime (assets/ next to the executable).
# The "runtime" component is what gets packaged, below, so files that bundled dependencies (such
# as cpr) install for their own use are not swept into the release archive with them.
install(TARGETS a-t RUNTIME DESTINATION . COMPONENT runtime)
install(DIRECTORY assets/ DESTINATION assets COMPONENT runtime)

set(CPACK_PACKAGE_NAME "a-t")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
# The actual archive is named <this>-runtime.tar.gz: CPack appends the component name for a
# component-based archive. "runtime" also reads fine as a name, so this is left as is rather
# than fought.
set(CPACK_PACKAGE_FILE_NAME "a-t-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL runtime)
# Component archives use CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY, not the plain
# CPACK_INCLUDE_TOPLEVEL_DIRECTORY; without it, extracting drops a-t and assets/ straight into
# the current directory instead of a folder of their own.
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)

enable_testing()
add_executable(tests tests/celestial_test.cpp tests/config_test.cpp tests/constellation_test.cpp tests/event_log_test.cpp tests/star_catalog_test.cpp tests/ground_track_test.cpp tests/satellite_roster_test.cpp tests/pass_planner_test.cpp tests/passes_test.cpp tests/observer_test.cpp tests/format_test.cpp tests/tle_age_test.cpp tests/satellite_position_test.cpp tests/star_map_time_test.cpp tests/system_status_test.cpp tests/transmission_events_test.cpp tests/typing_effect_test.cpp tests/frames_test.cpp tests/geodetic_test.cpp tests/kepler_test.cpp tests/lunar_test.cpp tests/operating_rules_test.cpp tests/sgp4_init_test.cpp tests/sgp4_propagate_test.cpp tests/sgp4_secular_test.cpp tests/time_test.cpp tests/tle_test.cpp tests/topocentric_test.cpp tests/tracked_satellite_test.cpp tests/tle_cache_test.cpp tests/tle_source_test.cpp)
target_link_libraries(tests PRIVATE app core net GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)
