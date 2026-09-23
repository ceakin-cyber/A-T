#include "app/config.h"
#include "app/satellite_roster.h"
#include "core/constellation.h"
#include "core/star_catalog.h"
#include "core/time.h"
#include "ui/bloom_chain.h"
#include "ui/crt_tuning_panel.h"
#include "ui/dockspace.h"
#include "ui/event_log_panel.h"
#include "ui/framebuffer.h"
#include "ui/ground_track_panel.h"
#include "ui/header.h"
#include "ui/pass_panel.h"
#include "ui/screen_pass.h"
#include "ui/star_map_panel.h"
#include "ui/style.h"
#include "ui/tracker_panel.h"
#include "ui/watchlist_panel.h"

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
    // Sync to the display's refresh rate, so the render loop doesn't spin faster than the
    // monitor can show and the frame rate stays a meaningful, steady number.
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "GL loader failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';

    const app::Config config = app::LoadConfig(app::DefaultConfigDir() / "config.txt");

    // Every watchlist entry is loaded and kept live; the SATELLITES panel selects which one the
    // other panels show. If an entry fails to load (offline with no cache) the app still runs,
    // with that row marked offline.
    app::SatelliteRoster roster(config.watchlist, config.observer);
    for (std::size_t i = 0; i < roster.Size(); ++i) {
        const app::WatchedSatellite& watched = roster.At(i);
        if (watched.satellite) {
            std::cout << "Tracking " << watched.satellite->tle.name << " ["
                      << watched.satellite->tle.catalogNumber << "], TLE epoch "
                      << watched.satellite->tle.epochYear << " day "
                      << watched.satellite->tle.epochDay << ", from "
                      << net::ToString(watched.satellite->source) << '\n';
        } else {
            std::cerr << "No data for [" << watched.entry.noradId << "] " << watched.entry.name
                      << '\n';
        }
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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

    // The UI is drawn into this offscreen target, which is then copied to the window.
    ui::Framebuffer sceneBuffer;

    // Draws that target to the window through a shader. If the shaders cannot be loaded, the
    // target is copied across as it is instead.
    ui::ScreenPass screenPass;
    ui::CrtSettings crtSettings;
    bool tuningOpen = false; // F2 toggles the CRT tuning panel
    std::cout << "Press F2 to tune the CRT effect\n";
    ui::BloomChain bloomChain;
    const bool haveScreenPass = screenPass.Load(exeDir / "assets" / "shaders") &&
                                bloomChain.Load(exeDir / "assets" / "shaders");
    if (!haveScreenPass) {
        std::cerr << "Shader pass unavailable, drawing without post-processing\n";
    }

    // Loaded once; magnitude filtering is already done in the shipped catalog file.
    const std::vector<core::Star> starCatalog =
        core::LoadStarCatalog(exeDir / "assets" / "stars" / "hygdata_mag6.csv");
    std::cout << "Star catalog: " << starCatalog.size() << " stars\n";

    const std::vector<core::ConstellationLine> constellationLines =
        core::LoadConstellationLines(exeDir / "assets" / "stars" / "constellation_lines.csv");
    std::cout << "Constellation lines: " << constellationLines.size() << " segments\n";

    while (!glfwWindowShouldClose(window)) {
        const net::Clock::time_point now = std::chrono::system_clock::now();
        roster.Update(now);
        const app::WatchedSatellite& selected = roster.Selected();

        const double julianDate = core::JulianDateFromTimePoint(now);
        const double lst = core::LocalSiderealTime(julianDate, config.observer.longitude);
        const std::vector<core::VisibleStar> visibleStars =
            core::VisibleStars(starCatalog, config.observer.latitude, lst);
        const std::vector<core::VisibleConstellationLine> visibleConstellationLines =
            core::VisibleConstellationLines(starCatalog, constellationLines,
                                            config.observer.latitude, lst);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const app::TrackedSatellite* selectedSatellite =
            selected.satellite ? &*selected.satellite : nullptr;
        const float headerHeight = ui::DrawHeaderBar(selectedSatellite);
        ui::DrawDockSpace(headerHeight);

        ui::DrawWatchlistPanel(roster);
        ui::DrawTrackerPanel(selectedSatellite, selected.position, now, headerHeight);
        if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
            tuningOpen = !tuningOpen;
        }
        ui::DrawCrtTuningPanel(crtSettings, tuningOpen, headerHeight);
        ui::DrawPassPanel(selectedSatellite, selected.nextPass, config.observer, now, headerHeight);
        ui::DrawGroundTrackPanel(&selected, config.observer);
        ui::DrawStarMapPanel(visibleStars, visibleConstellationLines);
        ui::DrawEventLogPanel(selected.eventLog);
        ImGui::Render();

        // A minimised window has no pixels to draw into; skip drawing until it comes back.
        if (sceneBuffer.Resize(width, height)) {
            sceneBuffer.Bind();
            const ImVec4 background = ui::ScreenBackground();
            glClearColor(background.x, background.y, background.z, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            ui::Framebuffer::Unbind();
            if (haveScreenPass) {
                if (crtSettings.bloomIntensity > 0.0F) {
                    bloomChain.Build(sceneBuffer.Texture(), width, height);
                }
                screenPass.Draw(sceneBuffer.Texture(), width, height, crtSettings, bloomChain);
            } else {
                sceneBuffer.BlitToScreen();
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Free GL objects while the context still exists.
    bloomChain.Release();
    screenPass.Release();
    sceneBuffer.Resize(0, 0);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
