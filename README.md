# A-T

A retro terminal-style satellite tracker, written in C++20 with OpenGL and Dear ImGui. It runs its own SGP4 orbit propagator, so positions come from the raw orbital elements and not from a third-party tracking API.

![A-T dashboard: ISS tracker, next pass and event log panels, with a scanline and glow CRT effect](docs/screenshot.png)

> **Status: in development.** The dashboard — tracker, pass predictor, event log, CRT shader and header status — is assembled and working. The star map and the remaining status panels (signal quality, lunar alignment, archives, and the rest) are still to come. See [Roadmap](#roadmap).

## What works today

- **Live ISS tracker:** latitude, longitude, altitude and speed, recomputed every frame from the system clock, plus a TLE age indicator that turns amber after 3 days and red after 7.
- **Next pass panel:** for a fixed observer, the rise, peak and set times (UTC), the peak elevation, the pass length, and a live countdown. If a pass is already under way it shows when it sets.
- **Event log:** logs real `SIGNAL ACQUIRED` / `SIGNAL LOST` transitions as the satellite rises and sets.
- **Header status:** `NODE: ONLINE` or `OFFLINE`, and `MODE: LIVE`, `CACHED` or `LOW-VISIBILITY`, reflecting whether the TLE came from the network, a fresh cache, or a stale one after a failed fetch.
- **Dockable dashboard:** the panels above are arranged with a real ImGui dockspace, so they can be dragged, resized and rearranged; your layout is remembered between runs.
- **CRT post-processing:** the picture is rendered to a texture and passed through scanline, glow and vignette shaders. Press **F2** in the running app to tune all three live and see the frame cost.
- **Orbital data:** the TLE comes from [Celestrak](https://celestrak.org/) and is cached on disk for two hours. If the network fails, the app falls back to a stale cached copy. With no network and no cache it still starts, and the panels show `NO DATA`.
- **Terminal look:** the VT323 pixel font, a green phosphor palette, and flat, square, minimal-border windows throughout.

The observer's location is a placeholder at Greenwich (51.4779 N, 0.0 E). Change `kObserver` in `src/app/observer.h` to your own. A config file is planned.

## Planned

- **Star map:** the real sky for the observer's location and time from the HYG catalog, with constellation lines and satellites overlaid as moving markers.
- **More panels:** signal quality from the NOAA planetary Kp index, lunar phase and alignment, a relay queue, archive status and a SQLite-backed archive, and operating rules read from a config file.
- **Stretch:** multiple tracked satellites, a config file for observer location and watchlist, CI running the test suite, a packaged release binary, and SDP4 support for deep-space satellites.

## Building

Requires a C++20 compiler, CMake 3.20 or newer, and an OpenGL-capable desktop. It is developed on Linux (Debian on WSL2). macOS and Windows are untested.

On Debian or Ubuntu, install the system libraries first:

```sh
sudo apt install build-essential cmake pkg-config libcurl4-openssl-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev \
    libwayland-dev libxkbcommon-dev wayland-protocols
```

GLFW, glad, Dear ImGui, cpr and GoogleTest are downloaded by CMake with `FetchContent`, so nothing else needs installing.

```sh
cmake -B build
cmake --build build
./build/a-t
```

Run the tests with:

```sh
ctest --test-dir build --output-on-failure
```

The build copies `assets/` next to the executable, so it can be run from any directory. On startup the app prefers the X11 window backend, because under WSLg the Wayland backend draws no title bar or window buttons. It falls back to any other backend if X11 is unavailable.

## Design

The code is layered so that the astronomy has no dependency on the network or the UI:

- `src/core/` is the astronomy library: time utilities (Julian date, sidereal time), the TLE parser, the SGP4 propagator, coordinate transforms (TEME to ECEF to geodetic, and observer-relative azimuth and elevation), and pass finding.
- `src/net/` fetches TLEs from Celestrak, caches them on disk, and falls back to stale data on failure.
- `src/app/` joins the two: it loads a satellite, computes its position, plans passes, logs events, and formats values for display.
- `src/ui/` is the GLFW, OpenGL and Dear ImGui front end: the style, the dockable dashboard, the panels, and the CRT post-processing shaders.
- `assets/shaders/` holds the GLSL for the post-processing pass, loaded at runtime, so tuning an effect only needs a restart, not a rebuild.
- `tests/` is the GoogleTest suite.

## Verification

The propagator and the coordinate code are checked against independent references, not only against themselves:

- **SGP4:** positions and velocities match [python-sgp4](https://pypi.org/project/sgp4/) to within 1 mm across seven TLEs and times from a week before to a week after the epoch. The expected values were generated once offline and hardcoded. Vallado's published verification vectors for satellite 00005 are also checked.
- **Time and Kepler's equation:** worked examples from Meeus's *Astronomical Algorithms* and Vallado's textbook.
- **Coordinates:** reference points computed independently in high-precision arithmetic.
- **Passes:** rise, set and peak times agree with an independent calculation to within 0.02 s.
- **End to end:** on 2026-09-20 the ISS position agreed with the separate tracker at wheretheiss.at to 2.4 km, entirely along the direction of travel. That comparison is kept as an offline test.

## Limitations

- Only near-Earth satellites (orbital period under 225 minutes) are supported. Deep-space orbits need the SDP4 model, which is not implemented, and are rejected.
- Polar motion is ignored, and UT1 is taken to equal UTC. Both are far smaller than the error in the orbital elements themselves.
- Atmospheric refraction is not applied to elevations.
- SGP4 accuracy falls with the age of the TLE, by roughly 1 to 3 km per day for the ISS.
- The TLE is fetched before the window opens. With no network and no cache, startup can wait up to 5 seconds for the request to time out.

## Roadmap

Work is tracked as GitHub milestones, each split into small single-purpose issues:

| # | Milestone | Status |
|---|-----------|--------|
| 0 | Repo and scaffolding | Done |
| 1 | Window and rendering skeleton | Done |
| 2 | Visual style pass | Done |
| 3 | TLE data layer | Done |
| 4 | Core SGP4 propagator | Done |
| 5 | Live tracker panel | Done |
| 6 | Pass predictor panel | Done |
| 7 | Shader / post-processing pass | Done |
| 8 | Full dashboard assembly | Done |
| 9 | Stretch: multiple satellites, config file, ground track, CI tests, release packaging, SDP4 deep-space support | Planned |
| 10 | System readout | Planned |
| 11 | Incoming transmission | Planned |
| 12 | Lunar alignment | Planned |
| 13 | Operating rules | Planned |
| 14 | Signal quality | Planned |
| 15 | Relay queue | Planned |
| 16 | Event log (full: severity, ring buffer, persistence) | Planned |
| 17 | Archive status | Planned |
| 18 | Archives | Planned |
| — | Realistic star map | Next |

## Credits and data sources

- SGP4 model: Hoots and Roehrich, *Spacetrack Report No. 3* (1980), and Vallado, Crawford, Hujsak and Kelso, *Revisiting Spacetrack Report #3* (2006).
- Time and astronomy algorithms: Meeus, *Astronomical Algorithms*, and Vallado, *Fundamentals of Astrodynamics and Applications*.
- Orbital elements: [Celestrak](https://celestrak.org/).
- Reference tracker used for the end-to-end check: [wheretheiss.at](https://wheretheiss.at/).
- Planned data sources: [NOAA Space Weather Prediction Center](https://www.swpc.noaa.gov/) and the [HYG Database](https://github.com/astronexus/HYG-Database).
- Font: [VT323](https://fonts.google.com/specimen/VT323), under the SIL Open Font License (see `assets/fonts/OFL.txt`).
- Libraries: Dear ImGui, GLFW, glad, cpr, libcurl and GoogleTest.

## License

MIT. See [LICENSE](LICENSE).
