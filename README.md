# A-T

A retro terminal-style satellite tracker and sky dashboard, written in C++20 with OpenGL and Dear ImGui. It runs its own SGP4 orbit propagator, so positions come from the raw orbital elements and not from a third-party tracking API.

> **Status: early development.** The repo is at the scaffolding stage and none of the features below are built yet. This README describes the planned design and roadmap. Sections marked *planned* will change as the code lands.

## What it will do

- **Live satellite tracker** (*planned*): shows the ISS's latitude, longitude and altitude in real time, plus how stale the loaded TLE is.
- **Pass predictor** (*planned*): finds the next rise, maximum elevation and set for a fixed observer location.
- **Realistic star map** (*planned*): draws the real sky for the observer's location and time from the HYG catalog, with constellation lines. Tracked satellites are overlaid as moving markers against the star field.
- **Signal quality** (*planned*): reads the live planetary Kp index from NOAA SWPC and reports `STABLE`, `DEGRADED` or `DISRUPTED`.
- **Lunar alignment** (*planned*): shows moon phase, illumination, age and the next full or new moon, computed locally.
- **System readout, incoming transmission, relay queue, archive status and event log** (*planned*): status panels driven by real application state. For example, the relay queue reflects the last fetch result, cache validity and pass-prediction status.
- **Archives** (*planned*): SQLite-backed history of events and completed pass predictions.
- **CRT post-processing** (*planned*): the UI is rendered to a texture and passed through scanline, bloom and vignette shaders.

## Design

- `src/core/` contains the astronomy code: time utilities (Julian date, GMST), the SGP4 propagator, TEME → ECEF → geodetic and topocentric transforms, pass finding, lunar phase, and star coordinate transforms. It is a standalone library with no UI or network dependencies, and it is unit-tested against reference values.
- `src/net/` fetches TLEs from [Celestrak](https://celestrak.org/) and the Kp index from NOAA SWPC. Responses are cached on disk, with a fallback to cached data when the network fails.
- `src/ui/` holds the GLFW, OpenGL and Dear ImGui front end and the post-processing shaders.
- `tests/` holds the GoogleTest suite. Propagator reference values are generated once offline with [python-sgp4](https://pypi.org/project/sgp4/) and hardcoded as expected results.

## Building

Requires a C++20 compiler, CMake, and an OpenGL-capable system. Dependencies (GLFW, glad, Dear ImGui, cpr, GoogleTest) are fetched through CMake `FetchContent`.

```sh
cmake -B build
cmake --build build
```

These commands work today for the placeholder executable. They won't produce a real application until the later milestones are done.

## Roadmap

Work is tracked as GitHub milestones, each split into small single-purpose issues:

| # | Milestone |
|---|-----------|
| 0 | Repo and scaffolding |
| 1 | Window and rendering skeleton |
| 2 | Visual style pass |
| 3 | TLE data layer |
| 4 | Core SGP4 propagator |
| 5 | Live tracker panel |
| 6 | Pass predictor panel |
| 7 | Shader / post-processing pass |
| 8 | Full dashboard assembly |
| 9 | Stretch: multiple satellites, config file, ground track, CI tests, release packaging, SDP4 deep-space support |
| 10 | System readout |
| 11 | Incoming transmission |
| 12 | Lunar alignment |
| 13 | Operating rules |
| 14 | Signal quality |
| 15 | Relay queue |
| 16 | Event log |
| 17 | Archive status |
| 18 | Archives |
| — | Realistic star map |

## Credits and data sources

- SGP4 model: Hoots and Roehrich, *Spacetrack Report No. 3* (1980), and Vallado, Crawford, Hujsak and Kelso, *Revisiting Spacetrack Report #3* (2006).
- Orbital elements: [Celestrak](https://celestrak.org/).
- Space weather: [NOAA Space Weather Prediction Center](https://www.swpc.noaa.gov/).
- Star catalog: [HYG Database](https://github.com/astronexus/HYG-Database).
- Dear ImGui, GLFW, glad, cpr and GoogleTest.

## License

MIT. See [LICENSE](LICENSE).
