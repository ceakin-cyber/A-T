#include "app/config.h"
#include "app/event_log.h"
#include "app/operating_rules.h"
#include "app/received_transmissions.h"
#include "app/relay_queue.h"
#include "app/satellite_roster.h"
#include "app/time_zone.h"
#include "app/star_map_time.h"
#include "app/system_status.h"
#include "app/transmission_events.h"
#include "core/constellation.h"
#include "core/land.h"
#include "core/lunar.h"
#include "core/star_catalog.h"
#include "core/time.h"
#include "net/kp_index_source.h"
#include "ui/bloom_chain.h"
#include "ui/crt_tuning_panel.h"
#include "ui/dockspace.h"
#include "ui/event_log_panel.h"
#include "ui/framebuffer.h"
#include "ui/ground_track_panel.h"
#include "ui/header.h"
#include "ui/incoming_transmission_panel.h"
#include "ui/lunar_alignment_panel.h"
#include "ui/operating_rules_panel.h"
#include "ui/pass_panel.h"
#include "ui/relay_queue_panel.h"
#include "ui/screen_pass.h"
#include "ui/signal_quality_panel.h"
#include "ui/star_map_panel.h"
#include "ui/style.h"
#include "ui/system_status_panel.h"
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
#include <optional>

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
    if (!app::SetDisplayTimeZone(config.timeZone)) {
        std::cerr << "Unknown time zone \"" << config.timeZone
                  << "\" in config; showing times in UTC\n";
    }

    // Every watchlist entry is loaded and kept live; the SATELLITES panel selects which one the
    // other panels show. If an entry fails to load (offline with no cache) the app still runs,
    // with that row marked offline.
    app::SatelliteRoster roster(config.watchlist, config.observer);
    std::vector<net::TleSource> loadedSources;
    std::vector<net::Clock::time_point> fetchTimes;

    // Real events from the app's own pipeline, across every watched satellite -- separate from
    // each satellite's own EVENT LOG (signal acquired/lost). See ui/incoming_transmission_panel.h.
    app::EventLog transmissionLog;
    const net::Clock::time_point startupTime = std::chrono::system_clock::now();

    for (std::size_t i = 0; i < roster.Size(); ++i) {
        const app::WatchedSatellite& watched = roster.At(i);
        if (watched.satellite) {
            std::cout << "Tracking " << watched.satellite->tle.name << " ["
                      << watched.satellite->tle.catalogNumber << "], TLE epoch "
                      << watched.satellite->tle.epochYear << " day "
                      << watched.satellite->tle.epochDay << ", from "
                      << net::ToString(watched.satellite->source) << '\n';
            loadedSources.push_back(watched.satellite->source);
            fetchTimes.push_back(watched.satellite->fetchedAt);

            // Both real: MakeSatellite (via the roster's loader) only reaches here once the TLE
            // has actually been obtained (network or cache) and the SGP4 model built from it.
            transmissionLog.Add(startupTime, "TLE FETCHED: " + watched.satellite->tle.name);
            transmissionLog.Add(startupTime,
                                "PROPAGATOR INITIALIZED: " + watched.satellite->tle.name);
        } else {
            std::cerr << "No data for [" << watched.entry.noradId << "] " << watched.entry.name
                      << '\n';
        }
    }

    // The station's own identity and health, for the SYSTEM STATUS panel. Computed once here,
    // right after the roster above finishes loading: nothing in this app's current, synchronous
    // startup changes any of this afterward (SatelliteRoster::Update never re-attempts a load
    // that failed the first time).
    app::SystemStatus systemStatus;
    systemStatus.callsign = app::kCallsign;
    systemStatus.nodeId = app::kNodeId;
    systemStatus.state = app::SystemState(!loadedSources.empty());
    systemStatus.mode = app::SystemMode(loadedSources);
    systemStatus.lastSync = app::LastSync(fetchTimes).value_or(net::Clock::time_point{});
    std::cout << "System status: " << systemStatus.state << ", mode " << systemStatus.mode
              << '\n';

    // The RELAY QUEUE's CARRIER PING, from how each watched satellite's TLE load went: computed
    // once here for the same reason as the system status above.
    std::vector<std::optional<net::TleSource>> tleResults;
    for (std::size_t i = 0; i < roster.Size(); ++i) {
        const app::WatchedSatellite& watched = roster.At(i);
        tleResults.push_back(watched.satellite ? std::optional(watched.satellite->source)
                                               : std::nullopt);
    }
    const app::RelayItem carrierPing = app::CarrierPing(tleResults);

    // The real Kp index behind the SIGNAL QUALITY panel, loaded once at startup like the TLEs
    // above: from a fresh cache if there is one, else NOAA, else a stale cache (see
    // net::LoadKpIndex). Nullopt only if all three fail; the panel then shows NO DATA.
    const std::optional<net::LoadedKp> kpIndex = net::LoadKpIndex();

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
    bool showConstellationLines = true; // the STAR MAP panel's own checkbox toggles this
    bool showStarLabels = false;        // ditto, for bright/named star labels; off by default,
                                        // since even the brightest/best-known stars alone can
                                        // crowd a small panel with no label collision handling
    app::StarMapTime starMapTime; // follows the real clock until the STAR MAP panel's own time
                                  // controls detach it; never affects any other panel

    // The LUNAR ALIGNMENT panel's snapshot: recomputed here at startup and again whenever the
    // panel's own REFRESH button is clicked below, never on every frame -- a deliberate manual-
    // refresh design, not a performance one (a live recompute would be cheap; see core/lunar.h).
    double lunarJulianDate = 0.0;
    core::LunarPhase lunarPhase;
    std::optional<double> nextNewMoonJd;
    std::optional<double> nextFullMoonJd;
    net::Clock::time_point lunarRefreshedAt{};
    const auto refreshLunarSnapshot = [&](net::Clock::time_point at) {
        lunarJulianDate = core::JulianDateFromTimePoint(at);
        lunarPhase = core::LunarPhaseAt(lunarJulianDate);
        nextNewMoonJd = core::NextNewMoon(lunarJulianDate);
        nextFullMoonJd = core::NextFullMoon(lunarJulianDate);
        lunarRefreshedAt = at;
    };
    refreshLunarSnapshot(startupTime); // an initial snapshot, so the panel isn't empty at launch
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

    // The GROUND TRACK map's continents, cut into triangles once here (after ImGui is set up,
    // which the triangulation borrows) so the map can fill them in cheaply every frame.
    const ui::LandMesh landMesh =
        ui::BuildLandMesh(core::LoadLandOutlines(exeDir / "assets" / "world" / "land_110m.txt"));
    std::cout << "Land outlines: " << landMesh.pieces.size() << " loaded\n";

    // Loaded once for the static OPERATING RULES panel; edit the file and restart to change it
    // (see assets/operating_rules.txt), no in-app reload.
    const std::vector<std::string> operatingRules =
        app::LoadOperatingRules(exeDir / "assets" / "operating_rules.txt");
    std::cout << "Operating rules: " << operatingRules.size() << " loaded\n";

    // The station's messages home, sent one at a time on a schedule and shown in the RELAY QUEUE
    // panel; edit assets/relay_messages.txt and restart to change them.
    app::RelayQueue relayQueue(app::LoadRelayMessages(exeDir / "assets" / "relay_messages.txt"),
                               startupTime);

    // Messages received on a schedule, for the INCOMING TRANSMISSION feed: the other half of the
    // relay queue's messages home. Edit assets/received_messages.txt and restart to change them.
    app::ReceivedTransmissions receivedTransmissions(
        app::LoadReceivedMessages(exeDir / "assets" / "received_messages.txt"), startupTime);

    // Built once and reused every frame, rather than re-indexing the whole catalog each time.
    const core::StarHipIndex starHipIndex(starCatalog);

    // The most recently seen next-pass per watched satellite, to notice (via app::PassChanged)
    // when PassPlanner has produced a genuinely new one, worth a "PASS COMPUTED" transmission
    // event -- indexed alongside the roster itself, which never changes size after construction.
    std::vector<std::optional<core::Pass>> lastLoggedPass(roster.Size());

    while (!glfwWindowShouldClose(window)) {
        const net::Clock::time_point now = std::chrono::system_clock::now();
        roster.Update(now);
        const app::WatchedSatellite& selected = roster.Selected();

        for (std::size_t i = 0; i < roster.Size(); ++i) {
            const app::WatchedSatellite& watched = roster.At(i);
            if (app::PassChanged(lastLoggedPass[i], watched.nextPass)) {
                lastLoggedPass[i] = watched.nextPass;
                if (watched.nextPass && watched.satellite) {
                    transmissionLog.Add(now, "PASS COMPUTED: " + watched.satellite->tle.name);
                }
            }
        }

        relayQueue.Update(now);
        if (const std::optional<std::string> message = receivedTransmissions.Due(now)) {
            transmissionLog.Add(now, *message);
        }

        // A fallback heartbeat once the feed above has gone quiet for a while (see IsIdle's own
        // comment on why this does not repeat every frame once logged).
        const std::optional<net::Clock::time_point> lastTransmission =
            transmissionLog.Entries().empty()
                ? std::nullopt
                : std::optional(transmissionLog.Entries().back().time);
        if (app::IsIdle(lastTransmission, now)) {
            transmissionLog.Add(now, "PASSIVE MONITORING ENGAGED");
        }

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // The star map's own clock: normally `now`, but the panel's time controls (drawn below)
        // can detach and move it, which is why this has to run after NewFrame -- it needs this
        // frame's real elapsed time (DeltaTime) to advance playback.
        app::Advance(starMapTime, now, ImGui::GetIO().DeltaTime);
        const net::Clock::time_point starMapNow = app::Effective(starMapTime, now);
        const double julianDate = core::JulianDateFromTimePoint(starMapNow);
        const double lst = core::LocalSiderealTime(julianDate, config.observer.longitude);
        const std::vector<core::VisibleStar> visibleStars =
            core::VisibleStars(starCatalog, config.observer.latitude, lst);
        const std::vector<core::VisibleConstellationLine> visibleConstellationLines =
            core::VisibleConstellationLines(starHipIndex, constellationLines,
                                            config.observer.latitude, lst);

        const app::TrackedSatellite* selectedSatellite =
            selected.satellite ? &*selected.satellite : nullptr;
        const float headerHeight = ui::DrawHeaderBar(selectedSatellite);
        ui::DrawDockSpace(headerHeight);

        ui::DrawWatchlistPanel(roster);
        ui::DrawSystemStatusPanel(systemStatus, headerHeight);
        ui::DrawSignalQualityPanel(kpIndex);
        ui::DrawTrackerPanel(selectedSatellite, selected.position, now, headerHeight);
        if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
            tuningOpen = !tuningOpen;
        }
        ui::DrawCrtTuningPanel(crtSettings, tuningOpen, headerHeight);
        ui::DrawPassPanel(selectedSatellite, selected.nextPass, config.observer, now, headerHeight);
        ui::DrawGroundTrackPanel(&selected, config.observer, landMesh, now);
        ui::DrawStarMapPanel(visibleStars, visibleConstellationLines, showConstellationLines,
                            showStarLabels, starMapTime, now);
        ui::DrawEventLogPanel(selected.eventLog);
        ui::DrawIncomingTransmissionPanel(transmissionLog, now);
        ui::DrawRelayQueuePanel(carrierPing, relayQueue, now);
        if (ui::DrawLunarAlignmentPanel(lunarPhase, lunarJulianDate, nextNewMoonJd, nextFullMoonJd,
                                        lunarRefreshedAt, config.observer.latitude)) {
            refreshLunarSnapshot(now);
        }
        ui::DrawOperatingRulesPanel(operatingRules);
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
