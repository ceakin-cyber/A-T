#include "app/observer.h"
#include "app/pass_planner.h"
#include "app/satellite_position.h"
#include "app/tracked_satellite.h"
#include "ui/header.h"
#include "ui/pass_panel.h"
#include "ui/style.h"
#include "ui/tracker_panel.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

int main(int /*argc*/, char** argv) {
    // Prefer X11: under WSLg the Wayland backend has no title bar or window buttons.
    // If X11 is unavailable (macOS, Windows, pure Wayland), fall back to any platform.
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    if (!glfwInit()) {
        glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
        if (!glfwInit()) {
            std::cerr << "GLFW init failed\n";
            return 1;
        }
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "A-T", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "GL loader failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';

    // ISS. If this fails (offline with no cache) the app still runs, just without a satellite.
    const std::optional<app::TrackedSatellite> satellite = app::LoadSatellite(25544);
    if (satellite) {
        std::cout << "Tracking " << satellite->tle.name << " [" << satellite->tle.catalogNumber
                  << "], TLE epoch " << satellite->tle.epochYear << " day "
                  << satellite->tle.epochDay << ", from " << net::ToString(satellite->source)
                  << '\n';
    } else {
        std::cerr << "No satellite to track\n";
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ui::ApplyTerminalStyle();

    // Assets are copied next to the executable by CMake.
    const std::filesystem::path exeDir = std::filesystem::absolute(argv[0]).parent_path();
    const std::string fontPath = (exeDir / "assets" / "fonts" / "VT323-Regular.ttf").string();
    ImGuiIO& io = ImGui::GetIO();
    if (!io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 20.0F)) {
        std::cerr << "Could not load font " << fontPath << ", using default\n";
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    std::optional<app::PassPlanner> planner;
    if (satellite) {
        planner.emplace(satellite->model, app::kObserver);
    }

    while (!glfwWindowShouldClose(window)) {
        const net::Clock::time_point now = std::chrono::system_clock::now();
        std::optional<app::SatellitePosition> position;
        std::optional<core::Pass> nextPass;
        if (satellite) {
            position = app::ComputePosition(*satellite, now);
            nextPass = planner->Next(now);
        }

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.0F, 0.02F, 0.01F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const float headerHeight = ui::DrawHeaderBar();

        ui::DrawTrackerPanel(satellite ? &*satellite : nullptr, position, now, headerHeight);
        ui::DrawPassPanel(satellite ? &*satellite : nullptr, nextPass, app::kObserver, now,
                          headerHeight);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
